
#include "vesa.h"
#include "bioscall.h"
#include "vmem.h"
#include <string.h>
#include <stdio.h>
#include "ice.h"

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

/* Current read/write bank */
// int curBank;

/* Bank granularity adjust factor */
// unsigned int bankShift;

/* Old video mode number */
int oldMode;

int bpp;

/* Pointer to start of video memory */
uint32_t *screenPtr;

/* Direct bank switching function */
// void (*bankSwitch)(void);


uint16_t attributes = -1;
uint8_t memory_model = -1;
uint8_t planes = -1;





/* Set new read/write bank. We must set both Window A and Window B, as
* many VBE's have these set as separately available read and write
* windows. We also use a simple (but very effective) optimization of
* checking if the requested bank is currently active.
*/
// void setBank(int bank)
// {
// 	union REGS in,out;
// 	if (bank == curBank) return;

// 	/* Bank is already active */
// 	curBank = bank;

// 	/* Save current bank number */
// 	bank <<= bankShift;

// 	/* Adjust to window granularity */
// 	#ifdef DIRECT_BANKING
// 		setbxdx(0,bank);
// 		bankSwitch();
// 		setbxdx(1,bank);
// 		bankSwitch();
// 	#else
// 		in.x.ax = 0x4F05; in.x.bx = 0;
// 		in.x.dx = bank;
// 		int86(0x10, &in, &out);
// 		in.x.ax = 0x4F05; in.x.bx = 1;
// 		in.x.dx = bank;
// 		int86(0x10, &in, &out);
// 	#endif
// }


struct VbeInfoBlock *vbeInfo = (VbeInfoBlock*)0x3000;//&ctrls;
struct ModeInfoBlock *modeInfo = (ModeInfoBlock*)0x4000;//&infs;



uint16_t findMode(int x, int y, int d)
{
	// struct VbeInfoBlock ctrls;
	// struct ModeInfoBlock infs;
	struct VbeInfoBlock *ctrl = vbeInfo;//&ctrls;
	struct ModeInfoBlock *inf = modeInfo;//&infs;

	memset(ctrl, 0, sizeof(VbeInfoBlock));
	memset(inf, 0, sizeof(ModeInfoBlock));
	// mem_free_nolength(ctrl);
	// mem_free_nolength(inf);
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
	regs.ebp = 0x100;
	regs.esp = 0x100;
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
 
	modes = (uint16_t*)REALPTR(ctrl->VideoModePtr);
	for ( i = 0 ; modes[i] != 0xFFFF ; ++i ) {

	 //  	printf("mode %X\n", modes[i]);

		// printf("OK 1\n");

		memset(&regs, 0, sizeof(regs));
		regs.eax = 0x4F01;
		regs.ecx = (int)modes[i];
		regs.edi = ((int)inf);
		regs.es = (int)inf >> 16;
		regs.ebp = 0x100;
		regs.esp = 0x100;
		regs.eflags = 0x202;
		regs.ds = 0x18;
		// regs.es = 0x18;
		regs.fs = 0x18;
		regs.gs = 0x18;
		regs.ss = 0x18;

		// printf("eax = %X\n", regs.eax);
		do_bios_call(&regs, 0x10);
		// printf("OK 11\n");
		if ( REGX(regs.eax) != 0x004F ) continue;
		// printf("  OK 12");

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



void setVBEMode(int mode)
{
	// union REGS in,out;
	// in.x.ax = 0x4F02; in.x.bx = mode;
	// int86(0x10,&in,&out);
	struct bios_regs regs;
	memset(&regs, 0, sizeof(struct bios_regs));

	regs.eax = 0x4F02;
	regs.ebx = mode | 0x4000;
	// regs.ecx = 0x0000;
	// regs.edx = 0x0000;
	// regs.esi = 0x0000;
	regs.es = 0x0000;
	regs.edi = 0x5000;
	regs.ebp = 0x100;
	regs.esp = 0x100;
	regs.eflags = 0x202;
	regs.ds = 0x18;
	// regs.es = 0x18;
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	// printf("eax = %X\n", regs.eax);
	do_bios_call(&regs, 0x10);

	assert(REGX(regs.eax) == 0x004F);
	// printf("eax = %X\n", regs.eax);
	printf("setVBEMode done\n");

}


/* Plot a pixel (0xAARRGGBB format) at location (x,y) in specified color */
static inline void putPixel(int x, int y, uint32_t color)
{
	const uint8_t nb_bytes = bpp/8;
	long addr = (long)y * bytesperline + x * nb_bytes;
	//setBank((int)(addr >> 16));

	if (!screenPtr)return;
	if (x > xres || y > yres)return;

	unsigned char *ptr = (unsigned char *)screenPtr;

	for (int i = 0; i < nb_bytes; i++) {
		*(ptr + addr + i) = (color >> 8*i) & 0xFF;
	}
	// *(screenPtr + (addr & 0xFFFFFF)) = color;
	// screenPtr[addr] = color;
}

static inline void putPixelRGB(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t pixel;

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



// void set_vesa()
// {
// 	struct bios_regs regs;
// 	memset(&regs, 0, sizeof(struct bios_regs));

// 	struct VbeInfoBlock *vib = mem_alloc(512);

// 	regs.eax = 0x4F00;
// 	// regs.ebx = 0x0000;
// 	// regs.ecx = 0x0000;
// 	// regs.edx = 0x0000;
// 	// regs.esi = 0x0000;
// 	regs.edi = (int)vib;
// 	// regs.edi = 0x0000;
// 	// regs.ebp = 0x100;
// 	// regs.esp = 0x100;
// 	// regs.eflags = 0x202;
// 	regs.ds = 0x18;
// 	regs.es = (int)vib;
// 	// regs.es = 0x18;
// 	regs.fs = 0x18;
// 	regs.gs = 0x18;
// 	regs.ss = 0x18;

// 	// printf("eax = %X\n", regs.eax);
// 	do_bios_call(&regs, 0x10);

// 	assert(REGX(regs.eax) == 0x004F);
// 	// printf("eax = %X\n", regs.eax);

// 	// for (int i = 0; i < 20; i++) {
// 	// 	vib[i].
// 	// }

// 	mem_free_nolength(vib);
// }




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
	regs.ebp = 0x100;
	regs.esp = 0x100;
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
	// union REGS in,out;
	// struct SREGS segs;
	// char far *modeInfo = (char far *)&ModeInfoBlock;
	if (mode < 0x100) return 0;

	// /* Ignore non-VBE modes */
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
	regs.ebp = 0x100;
	regs.esp = 0x100;
	regs.eflags = 0x202;
	regs.ds = 0x18;
	// regs.es = 0x18;
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	do_bios_call(&regs, 0x10);

	if (REGX(regs.eax) != 0x004F) return 0;
	printf("regs.eax %X\n", regs.eax);

	attributes = modeInfo->attributes;
	memory_model = modeInfo->memory_model;
	planes = modeInfo->planes;

	printf("attributes %d  memory_model %d  planes %d\n", attributes, memory_model, planes);

	// if ((modeInfo->attributes & 0x1)
	// 	&& modeInfo->memory_model == memPK
	// 	&& modeInfo->bpp == 8
	// 	&& modeInfo->planes == 1)
	// 	return 1;

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
	regs.ebp = 0x100;
	regs.esp = 0x100;
	regs.eflags = 0x202;
	regs.ds = 0x18;
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	do_bios_call(&regs, 0x10);
	if ( REGX(regs.eax) != 0x004F ) return;

	modes = (uint16_t*)REALPTR(ctrl->VideoModePtr);
	for ( i = 0 ; modes[i] != 0xFFFF ; ++i ) {
		memset(&regs, 0, sizeof(regs));
		regs.eax = 0x4F01;
		regs.ecx = (int)modes[i];
		regs.edi = ((int)inf);
		regs.es = (int)inf >> 16;
		regs.ebp = 0x100;
		regs.esp = 0x100;
		regs.eflags = 0x202;
		regs.ds = 0x18;
		regs.fs = 0x18;
		regs.gs = 0x18;
		regs.ss = 0x18;

		do_bios_call(&regs, 0x10);
		if ( REGX(regs.eax) != 0x004F ) continue;

		if (inf->Xres > min && inf->Xres < max)
		printf("0x%X : %dx%dx%d\n", modes[i], inf->Xres, inf->Yres, inf->bpp);
	}
}


extern char pgdir[];
/* Initialize the specified video mode. Notice how we determine a shift
* factor for adjusting the Window granularity for bank switching. This
* is much faster than doing it with a multiply (especially with direct
* banking enabled).
*/
int initGraphics(unsigned int x, unsigned int y, unsigned int d)
{
	// uint16_t *p;

	// if (!getVbeInfo())
	// {
	// 	printf("No VESA VBE detected\n");
	// 	return;
	// }

	mode = findMode(x, y, d);

	// for (p = vbeInfo->VideoModePtr; *p != 0xFFFF; p++)
	// {
		// if (getModeInfo(*p) && modeInfo->XResolution == x
		// 	&& modeInfo->YResolution == y) {
		if (getModeInfo(mode)) {
			xres = modeInfo->Xres;
			yres = modeInfo->Yres;
			bpp = modeInfo->bpp;
			// bytesperline = modeInfo->BytesPerScanLine;
			bytesperline = modeInfo->LinBytesPerScanLine;
			// bankShift = 0;

			printf("mode : %dx%dx%d\n", xres, yres, bpp);
			screenPtr = (uint32_t*)modeInfo->physbase;
			printf("bytesperline : 0x%X\n", (int)bytesperline);
			printf("screenPtr : 0x%X\n", (int)screenPtr);
			// while (1);

			// while ((unsigned)(64 >> bankShift) != ModeInfoBlock.WinGranularity)
			// 	bankShift++;

			// bankSwitch = ModeInfoBlock.WinFuncPtr;

			// curBank = -1;
			// screenPtr = (char far *)( ((long)0xA000)<<16 | 0);
			// oldMode = getVBEMode();

			if (!screenPtr)
				return 0;

			setVBEMode(mode);

			uint32_t size = xres*yres * bytesperline;
			uint32_t pages = compute_pages(size);
			Process *cur_proc = get_cur_proc();
			void *pdir = pgdir;

			if (cur_proc && cur_proc->pdir)
				pdir = cur_proc->pdir;

			for (uint32_t i = 0; i < pages; i++) {
				void *addr = (void*)((int)screenPtr + i * PAGESIZE);
				map_page((void*)pdir, addr, addr, P_USERSUP | P_RW);
			}

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
			for (int32_t j = 200; j < 200+161; j++)
				for (int32_t i = 400; i < 288+400; i++) {

					if (k > ice_bin_size)break;

					const char b = ice_bin[k++];
					const char g = ice_bin[k++];
					const char r = ice_bin[k++];
					const char a = ice_bin[k++];
					(void)a;

					putPixelRGB(i, j, r, g, b);
				}

			// int32_t k = 0;
			// for (int32_t j = 0; j < 768; j++)
			// 	for (int32_t i = 0; i < 1366; i++) {

					// if (k > matrix_bin_size)break;

					// const char b = matrix_bin[k++];
					// const char g = matrix_bin[k++];
					// const char r = matrix_bin[k++];
					// const char a = matrix_bin[k++];
				// 	(void)a;

				// 	putPixelRGB(i, j, r, g, b);
				// }

			return (int)screenPtr;
		}
	// }

	printf("Video mode not found\n");

	return 0;
}


