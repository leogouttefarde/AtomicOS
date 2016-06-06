
#include "vesa.h"
#include "bioscall.h"
#include "vmem.h"
#include <string.h>
#include <stdio.h>
#include "ice.h"
#include "cursor.h"
#include "vmem.h"
#include "mem.h"

// #define VIDEO_MEMORY 0x60000000
#define VIDEO_MEMORY 0xC0000000

typedef struct VbeInfoBlock {
	 char VbeSignature[4];             // == "VESA"
	 uint16_t VbeVersion;                 // == 0x0300 for VBE 3.0
	 // uint16_t OemStringPtr[2];            // isa vbeFarPtr
	 uint32_t OemStringPtr;            // isa vbeFarPtr
	 uint8_t Capabilities[4];
	 // uint16_t VideoModePtr[2];         // isa vbeFarPtr
	 uint32_t VideoModePtr;         // isa vbeFarPtr
	 uint16_t TotalMemory;             // as # of 64KB blocks
} __attribute__((packed)) VbeInfoBlock;


typedef struct ModeInfoBlock {
	uint16_t attributes;
	uint8_t winA,winB;
	uint16_t granularity;
	uint16_t winsize;
	uint16_t segmentA, segmentB;
	int realFctPtr;
	// VBE_FAR(realFctPtr);
	uint16_t pitch; // bytes per scanline

	uint16_t Xres, Yres;
	uint8_t Wchar, Ychar, planes, bpp, banks;
	uint8_t memory_model, bank_size, image_pages;
	uint8_t reserved0;

	uint8_t red_mask, red_position;
	uint8_t green_mask, green_position;
	uint8_t blue_mask, blue_position;
	uint8_t rsv_mask, rsv_position;
	uint8_t directcolor_attributes;

	uint32_t physbase;  // your LFB (Linear Framebuffer) address ;)
	uint32_t reserved1;
	uint16_t reserved2;
	uint16_t LinBytesPerScanLine;
} __attribute__((packed)) ModeInfoBlock;



#define DIFF(a,b) ((a-b) > 0 ? a-b : b-a)
#define REALPTR(p) (p)


extern long int matrix_bin_size;
extern unsigned char matrix_bin[];



typedef enum {

	/*
	 Planar
	 memory model
	*/
	memPL = 3,

	/*
	 Packed
	 pixel memory model
	*/
	memPK = 4,

	/*
	 Direct
	 color RGB memory model
	*/
	memRGB = 6,

	/*
	 Direct
	 color YUV memory model
	*/
	memYUV = 7

} memModels;


int mode = 0;

char mystr[256];
char *get_str();

/* Resolution of video mode used */
int xres, yres;

/* Logical CRT scanline length */
int bytesperline;

/* Old video mode number */
int oldMode;

int bpp;

/* Pointer to start of video memory */
uint32_t *screenPtr = NULL;


uint16_t attributes = -1;
uint8_t memory_model = -1;
uint8_t planes = -1;



struct VbeInfoBlock *vbeInfo = (VbeInfoBlock*)0x3000;//&ctrls;
struct ModeInfoBlock *modeInfo = (ModeInfoBlock*)0x4000;//&infs;

bool changed = false;

uint16_t findMode(int x, int y, int d)
{
	struct VbeInfoBlock *ctrl = vbeInfo;
	struct ModeInfoBlock *inf = modeInfo;

	memset(ctrl, 0, sizeof(VbeInfoBlock));
	memset(inf, 0, sizeof(ModeInfoBlock));
	uint16_t *modes;
	int i;
	uint16_t best = 0x13;
	int pixdiff, bestpixdiff = DIFF(320 * 200, x * y);
	int depthdiff, bestdepthdiff = 8 >= d ? 8 - d : (d - 8) * 2;
 
	strncpy(ctrl->VbeSignature, "VBE2", 4);

	uint16_t cx = 320, cy = 200;


	struct bios_regs regs;
	memset(&regs, 0, sizeof(regs));

	regs.eax = 0x4F00;
	regs.edi = ((int)ctrl);
	regs.es = (int)ctrl>>16;
	regs.ebp = 0x5000;
	regs.esp = 0x5000;
	regs.eflags = 0x202;
	regs.ds = 0x18;
	// regs.es = 0x18;
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	printf("eax = %X\n", regs.eax);
	do_bios_call(&regs, 0x10);
	if ( REGX(regs.eax) != 0x004F ) return best;

	printf("OK 0  %X\n", ctrl->VideoModePtr);

	// intV86(0x10, "ax,es:di", 0x4F00, 0, ctrl); // Get Controller Info
	// if ( (uint16_t)v86.tss.eax != 0x004F ) return best;

	if (!ctrl->VideoModePtr)
		return best;
 
	modes = (uint16_t*)ctrl->VideoModePtr;
	for ( i = 0 ; modes[i] != 0xFFFF ; ++i ) {

		memset(&regs, 0, sizeof(regs));
		regs.eax = 0x4F01;
		regs.ecx = (int)modes[i];
		regs.edi = ((int)inf);
		regs.es = (int)inf >> 16;
		regs.ebp = 0x5000;
		regs.esp = 0x5000;
		regs.eflags = 0x202;
		regs.ds = 0x18;
		// regs.es = 0x18;
		regs.fs = 0x18;
		regs.gs = 0x18;
		regs.ss = 0x18;

		// printf("eax = %X\n", regs.eax);
		do_bios_call(&regs, 0x10);
		if ( REGX(regs.eax) != 0x004F ) continue;

		// Skip invalid modes
		if ( x < inf->Xres || y < inf->Yres ) continue;

		if (inf->Xres > 700)
		printf("m 0x%X x %d y %d d %d\n", modes[i], inf->Xres, inf->Yres, inf->bpp);

		// intV86(0x10, "ax,cx,es:di", 0x4F01, modes[i], 0, inf); // Get Mode Info
		// if ( (uint16_t)v86.tss.eax != 0x004F ) continue;

		// Check if this is a graphics mode with linear frame buffer support
		if ( (inf->attributes & 0x90) != 0x90 ) continue;

		// Check if this is a packed pixel or direct color mode
		if ( inf->memory_model != 4 && inf->memory_model != 6 ) continue;

		// Check if this is exactly the mode we're looking for
		if ( x == inf->Xres && y == inf->Yres &&
				d == inf->bpp ) return modes[i];

		// Otherwise, compare to the closest match so far, remember if best
		pixdiff = DIFF(inf->Xres * inf->Yres, x * y);
		depthdiff = (inf->bpp >= d)? inf->bpp - d : (d - inf->bpp) * 2;
		if ( /*inf->Xres > cx || inf->Yres > cy || */bestpixdiff > pixdiff ||
				(bestpixdiff == pixdiff && bestdepthdiff > depthdiff) ) {
			best = modes[i];
			bestpixdiff = pixdiff;
			bestdepthdiff = depthdiff;
			cx = inf->Xres;
			cy = inf->Xres;
			cx=cx;cy=cy;
		}
	}
	if ( x == 640 && y == 480 && d == 1 ) return 0x11;
	return best;
}

extern char pgdir[];

static inline void *get_pdir()
{
	const Process *cur_proc = get_cur_proc();
	void *pdir = pgdir;

	if (cur_proc && cur_proc->pdir)
		pdir = cur_proc->pdir;

	return pdir;
}


/* Plot a pixel (0xAARRGGBB format) at location (x,y) in specified color */
static inline void putPixel(int x, int y, uint32_t color)
{
	const uint8_t nb_bytes = bpp/8;
	long addr = (long)y * bytesperline + x * nb_bytes;

	if (!screenPtr || !changed)
		return;

	if (x > xres || y > yres)
		return;

	unsigned char *ptr = (unsigned char *)VIDEO_MEMORY;

	for (int i = 0; i < nb_bytes; i++) {
		*(ptr + addr + i) = (color >> 8*i) & 0xFF;
	}
}

static inline void putPixelRGB(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t pixel;

	// Treat magenta as transparency
	if (r == 255 && b == 255 && g == 0)
		return;

	if (bpp == 16) {
		r = (((uint32_t)r) * 32)/256;
		g = (((uint32_t)g) * 64)/256;
		b = (((uint32_t)b) * 32)/256;
		pixel = ((((r & 0b11111) << 6) | (g & 0b111111)) << 5) | (b & 0b11111);
	}
	else {
		pixel = (((r << 8) | g) << 8) | b | 0xFF000000;
	}

	pixel = pixel;

	putPixel(x, y, pixel);
}


static inline void map_video_memory()
{
	void *pdir = get_pdir();

	if (!screenPtr || get_physaddr(pdir, screenPtr))
		return;

	const uint32_t size = xres*yres * bytesperline;
	const uint32_t pages = compute_pages(size);

	for (uint32_t i = 0; i < pages; i++) {
		void *addr = (void*)((int)screenPtr + i * PAGESIZE);
		void *vaddr = (void*)(VIDEO_MEMORY + i * PAGESIZE);
		map_page((void*)pdir, addr, vaddr, P_USERSUP | P_RW);
	}
}



void set_vbe_mode(int mode)
{
	// union REGS in,out;
	// in.x.ax = 0x4F02; in.x.bx = mode;
	// int86(0x10,&in,&out);
	struct bios_regs regs;
	memset(&regs, 0, sizeof(struct bios_regs));

	regs.eax = 0x4F02;

	// Use linear framebuffer
	regs.ebx = mode | 0x4000; 

	// regs.ecx = 0x0000;
	// regs.edx = 0x0000;
	// regs.esi = 0x0000;
	regs.es = 0x0000;
	regs.edi = 0x5000;
	regs.ebp = 0x5000;
	regs.esp = 0x5000;
	regs.eflags = 0x202;
	regs.ds = 0x18;
	// regs.es = 0x18;
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	// printf("eax = %X\n", regs.eax);
	do_bios_call(&regs, 0x10);

	if (REGX(regs.eax) != 0x004F) {
		printf("set_vbe_mode failed\n");
	}
	else {
		changed = true;
		//printf("set_vbe_mode done\n");
	}
}

bool is_console_mode()
{
	return !changed;
}


void init_vbe_mode(int mode)
{
	if (!getModeInfo(mode))
		return;

	xres = modeInfo->Xres;
	yres = modeInfo->Yres;
	bpp = modeInfo->bpp;
	// bytesperline = modeInfo->BytesPerScanLine;
	bytesperline = modeInfo->LinBytesPerScanLine;

	printf("mode : %dx%dx%d\n", xres, yres, bpp);
	screenPtr = (uint32_t*)modeInfo->physbase;
	printf("bytesperline : 0x%X\n", (int)bytesperline);
	printf("screenPtr : 0x%X\n", (int)screenPtr);

	if (!screenPtr)
		return;

	set_vbe_mode(mode);

	// map_video_memory();

	// // Draw gradient bg
	// for (int32_t i = 0; i < xres; i++)
	// 	for (int32_t j = 0; j < yres; j++) {
	// 		// Format couleur : 0xAARRGGBB
	// 		// putPixel(i, j, 0xFF000000
	// 		// 	| (i%256) * 0x00010000
	// 		// 	| ((i+j)%256) * 0x00000100
	// 		// 	| (j%256) * 0x000000FF
	// 		// 	);
	// 		putPixelRGB(i, j, i, (i+j), j);
	// 		// putPixelRGB(i, j, 0xFF, 0, 0);
	// 	}

	// int32_t k = 0;
	// for (int32_t j = 200; j < 200+161; j++)
	// 	for (int32_t i = 400; i < 288+400; i++) {

	// 		if (k > ice_bin_size)break;

	// 		const char b = ice_bin[k++];
	// 		const char g = ice_bin[k++];
	// 		const char r = ice_bin[k++];
	// 		const char a = ice_bin[k++];
	// 		(void)a;

	// 		putPixelRGB(i, j, r, g, b);
	// 	}
}


/* Get SuperVGA information, returning true if VBE found */
int getVbeInfo()
{
	struct bios_regs regs;
	memset(&regs, 0, sizeof(struct bios_regs));

	regs.eax = 0x4F00;
	regs.edi = ((int)vbeInfo);
	regs.es = (int)vbeInfo>>16;
	// regs.ebx = 0x0000;
	// regs.ecx = 0x0000;
	// regs.edx = 0x0000;
	// regs.esi = 0x0000;
	// regs.edi = 0x0000;
	regs.ebp = 0x5000;
	regs.esp = 0x5000;
	regs.eflags = 0x202;
	regs.ds = 0x18;
	// regs.es = 0x18;
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	do_bios_call(&regs, 0x10);

	return REGX(regs.eax) == 0x004F;
}



/* Get video mode information given a VBE mode number. We return 0 if
* if the mode is not available, or if it is not a 256 color packed
* pixel mode.
*/
int getModeInfo(int mode)
{
	printf("modeInfo %X\n", mode);

	// /* Ignore non-VBE modes */
	if (mode < 0x100) return 0;

	// in.x.ax = 0x4F01;
	// in.x.cx = mode;
	// in.x.di = FP_OFF(modeInfo);
	// segs.es = FP_SEG(modeInfo);
	// int86x(0x10, &in, &out, &segs);
	struct bios_regs regs;
	memset(&regs, 0, sizeof(struct bios_regs));

	regs.eax = 0x4F01;
	regs.edi = ((int)modeInfo);
	regs.es = (int)modeInfo>>16;
	// regs.ebx = 0x0000;
	regs.ecx = mode;
	// regs.edx = 0x0000;
	// regs.esi = 0x0000;
	// regs.edi = 0x0000;
	regs.ebp = 0x5000;
	regs.esp = 0x5000;
	regs.eflags = 0x202;
	regs.ds = 0x18;
	// regs.es = 0x18;
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	do_bios_call(&regs, 0x10);

	if (REGX(regs.eax) != 0x004F)
		return 0;

	printf("regs.eax %X\n", regs.eax);

	attributes = modeInfo->attributes;
	memory_model = modeInfo->memory_model;
	planes = modeInfo->planes;

	printf("attributes %d  memory_model %d  planes %d\n", attributes, memory_model, planes);

	return 1;
}

void list_modes(int min, int max)
{
	struct VbeInfoBlock *ctrl = vbeInfo;
	struct ModeInfoBlock *inf = modeInfo;

	memset(ctrl, 0, sizeof(VbeInfoBlock));
	memset(inf, 0, sizeof(ModeInfoBlock));
	uint16_t *modes;
	int i;
 
	strncpy(ctrl->VbeSignature, "VBE2", 4);


	struct bios_regs regs;
	memset(&regs, 0, sizeof(regs));

	regs.eax = 0x4F00;
	regs.edi = ((int)ctrl);
	regs.es = (int)ctrl>>16;
	regs.ebp = 0x5000;
	regs.esp = 0x5000;
	regs.eflags = 0x202;
	regs.ds = 0x18;
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	do_bios_call(&regs, 0x10);

	if (REGX(regs.eax) != 0x004F)
		return;

	modes = (uint16_t*)ctrl->VideoModePtr;
	for ( i = 0 ; modes[i] != 0xFFFF ; ++i ) {
		memset(&regs, 0, sizeof(regs));
		regs.eax = 0x4F01;
		regs.ecx = (int)modes[i];
		regs.edi = ((int)inf);
		regs.es = (int)inf >> 16;
		regs.ebp = 0x5000;
		regs.esp = 0x5000;
		regs.eflags = 0x202;
		regs.ds = 0x18;
		regs.fs = 0x18;
		regs.gs = 0x18;
		regs.ss = 0x18;

		do_bios_call(&regs, 0x10);
		if (REGX(regs.eax) != 0x004F)
			continue;

		if (inf->Xres >= min && inf->Xres <= max)
		printf("0x%X : %dx%dx%d 0x%X\n", modes[i], inf->Xres, inf->Yres, inf->bpp, inf->physbase);
	}
}

int init_graphics(unsigned int x, unsigned int y, unsigned int d)
{
	mode = findMode(x, y, d);
	init_vbe_mode(mode);

	return (int)screenPtr;
}

void fill_rectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
	// Mouse events may not occur from
	// another process with a different memory mapping
	map_video_memory();
	(void)w;
	(void)h;
	(void)color;


	// Draw gradient bg
	for (int32_t i = 0; i < xres; i++)
		for (int32_t j = 0; j < yres; j++) {
			// Format couleur : 0xAARRGGBB
			// putPixel(i, j, 0xFF000000
			// 	| (i%256) * 0x00010000
			// 	| ((i+j)%256) * 0x00000100
			// 	| (j%256) * 0x000000FF
			// 	);
			putPixelRGB(i, j, i, (i+j), j);
			// putPixelRGB(i, j, 0xFF, 0, 0);
		}
	int32_t k = 0;
	// for (int32_t j = 200; j < 200+161; j++)
	// 	for (int32_t i = 400; i < 288+400; i++) {

	// 		if (k > ice_bin_size)break;

	// 		const char b = ice_bin[k++];
	// 		const char g = ice_bin[k++];
	// 		const char r = ice_bin[k++];
	// 		const char a = ice_bin[k++];
	// 		(void)a;

	// 		putPixelRGB(i, j, r, g, b);
	// 	}


	// for (uint32_t j = y; j < (y + h); j++) {
	// 	for (uint32_t i = x; i < (x + w); i++) {
	// 		putPixel(i, j, color);
	// 	}
	// }
	// int32_t k = 0;
	 k = 0;
	for (uint32_t j = y; j < y+24; j++)
		for (uint32_t i = x; i < 16+x; i++) {

			if (k > cursor_bin_size)break;

			const char b = cursor_bin[k++];
			const char g = cursor_bin[k++];
			const char r = cursor_bin[k++];
			const char a = cursor_bin[k++];
			(void)a;

			putPixelRGB(i, j, r, g, b);
		}
}

