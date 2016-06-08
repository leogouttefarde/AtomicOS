
#include "vesa.h"
#include "bioscall.h"
#include "vmem.h"
#include <string.h>
#include <stdio.h>
#include "images/cursor.h"
#include "images/bg.h"
#include "vmem.h"
#include "mem.h"
#include "file.h"
#include "screen.h"

// #define VIDEO_MEMORY 0x60000000
#define VIDEO_MEMORY 0xC0000000

#define BESTFIT(a,b) ((a-b) > 0 ? a-b : b-a)

typedef struct VbeInfoBlock {
	 char VbeSignature[4];
	 uint16_t VbeVersion;
	 uint32_t OemStringPtr;
	 uint8_t Capabilities[4];
	 uint32_t VideoModePtr;
	 uint16_t TotalMemory;
} __attribute__((packed)) VbeInfoBlock;

typedef struct ModeInfoBlock {
	uint16_t attributes;
	uint8_t winA,winB;
	uint16_t granularity;
	uint16_t winsize;
	uint16_t segmentA, segmentB;
	int realFctPtr;
	uint16_t pitch;

	uint16_t Xres, Yres;
	uint8_t Wchar, Ychar, planes, bpp, banks;
	uint8_t memory_model, bank_size, image_pages;
	uint8_t reserved0;

	uint8_t red_mask, red_position;
	uint8_t green_mask, green_position;
	uint8_t blue_mask, blue_position;
	uint8_t rsv_mask, rsv_position;
	uint8_t directcolor_attributes;

	uint32_t physbase;
	uint32_t reserved1;
	uint16_t reserved2;
	uint16_t LinBytesPerScanLine;
} __attribute__((packed)) ModeInfoBlock;


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


int g_mode = 0;

/* Resolution of video mode used */
uint32_t xres, yres;

/* Logical CRT scanline length */
int bytesperline;

/* Old video mode number */
int oldMode = 0;

int bpp;

/* Pointer to start of video memory */
uint32_t *screenPtr = NULL;

/* Pointer to video memory buffer */
uint32_t *screenBuf = NULL;


uint16_t attributes = -1;
uint8_t memory_model = -1;
uint8_t planes = -1;

// Real mode data pointers
struct VbeInfoBlock *vbeInfo = (VbeInfoBlock*)0x3000;
struct ModeInfoBlock *modeInfo = (ModeInfoBlock*)0x4000;


uint16_t findMode(int x, int y, int d)
{
	struct VbeInfoBlock *ctrl = vbeInfo;
	struct ModeInfoBlock *inf = modeInfo;

	memset(ctrl, 0, sizeof(VbeInfoBlock));
	memset(inf, 0, sizeof(ModeInfoBlock));
	uint16_t *modes;
	int i;
	uint16_t best = 0x13;
	int pixdiff, bestpixdiff = BESTFIT(320 * 200, x * y);
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
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	do_bios_call(&regs, 0x10);
	if ( REGX(regs.eax) != 0x004F ) return best;

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
		regs.fs = 0x18;
		regs.gs = 0x18;
		regs.ss = 0x18;

		do_bios_call(&regs, 0x10);
		if ( REGX(regs.eax) != 0x004F ) continue;

		// Skip invalid modes
		if ( !inf->physbase ) continue;
		if ( x < inf->Xres || y < inf->Yres ) continue;

		if (inf->Xres > 700)

		// Check if this is a graphics mode with linear frame buffer support
		if ( (inf->attributes & 0x90) != 0x90 ) continue;

		// Check if this is a packed pixel or direct color mode
		if ( inf->memory_model != 4 && inf->memory_model != 6 ) continue;

		// Check if this is exactly the mode we're looking for
		if ( x == inf->Xres && y == inf->Yres &&
				d == inf->bpp ) return modes[i];

		// Otherwise, compare to the closest match so far, remember if best
		pixdiff = BESTFIT(inf->Xres * inf->Yres, x * y);
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
static inline void putPixel(uint32_t x, uint32_t y, uint32_t color)
{
	const uint8_t nb_bytes = bpp/8;
	long addr = (long)y * bytesperline + x * nb_bytes;

	if (!screenPtr || is_console_mode())
		return;

	if (x > xres || y > yres)
		return;

	unsigned char *ptr = (unsigned char *)screenBuf;

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
		pixel = ((((r & 0b11111) << 6)
			| (g & 0b111111)) << 5)
			| (b & 0b11111);
	}
	else {
		pixel = (((r << 8) | g) << 8) | b | 0xFF000000;
	}

	putPixel(x, y, pixel);
}

static inline void map_video_memory()
{
	void *pdir = get_pdir();

	if (!screenPtr || get_physaddr(pdir, (void*)VIDEO_MEMORY))
		return;

	const uint32_t size = xres * yres * bytesperline;
	const uint32_t pages = compute_pages(size);
	void *addr, *vaddr;

	for (uint32_t i = 0; i < pages; i++) {
		addr = (void*)((int)screenPtr + i * PAGESIZE);
		vaddr = (void*)(VIDEO_MEMORY + i * PAGESIZE);

		map_page((void*)pdir, addr, vaddr, P_USERSUP | P_RW);
	}
}

char *img_data = NULL;
uint16_t img_width = 0;
uint16_t img_height = 0;

uint32_t get_screen_width()
{
	if (is_console_mode()) {
		return NB_COLS;
	}

	return xres;
}

uint32_t get_screen_height()
{
	if (is_console_mode()) {
		return NB_LIG;
	}

	return yres;
}

int get_vbe_mode()
{
	struct bios_regs regs;
	memset(&regs, 0, sizeof(struct bios_regs));

	regs.eax = 0x4F03;
	regs.ebp = 0x100;
	regs.esp = 0x100;
	regs.eflags = 0x202;
	regs.ds = 0x18;
	regs.es = 0x18;
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	do_bios_call(&regs, 0x10);

	if (REGX(regs.eax) != 0x004F) {
		printf("get_vbe_mode failed\n");
		return 0;
	}

	//printf("current mode : 0x%X\n", REGX(regs.ebx));

	return REGX(regs.ebx);
}

void set_vbe_mode(int mode)
{
	struct bios_regs regs;
	memset(&regs, 0, sizeof(struct bios_regs));

	regs.eax = 0x4F02;

	regs.ebx = mode;

	// Use linear framebuffer
	if (mode >= 0x100)
		regs.ebx |= 0x4000;

	regs.es = 0x0000;
	regs.edi = 0x5000;
	regs.ebp = 0x100;
	regs.esp = 0x100;
	regs.eflags = 0x202;
	regs.ds = 0x18;
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	if (!oldMode) {
		oldMode = get_vbe_mode();
	}

	do_bios_call(&regs, 0x10);

	if (REGX(regs.eax) != 0x004F) {
		printf("set_vbe_mode failed\n");
	}
	else {
		g_mode = mode;

		if (mode == oldMode) {
			screenPtr = NULL;
			img_data = NULL;
		}
		else {
			map_video_memory();
		}

		if (!screenBuf) {
			screenBuf = phys_alloc(0x800000);
		}
	}
}

bool is_console_mode()
{
	return (g_mode == oldMode);
}

void set_console_mode()
{
	if (oldMode) {
		set_vbe_mode(oldMode);
	}
}

void init_vbe_mode(int mode)
{
	if (!getModeInfo(mode) && mode >= 0x100)
		return;

	xres = modeInfo->Xres;
	yres = modeInfo->Yres;
	bpp = modeInfo->bpp;
	bytesperline = modeInfo->LinBytesPerScanLine;

	// printf("mode : %dx%dx%d\n", xres, yres, bpp);
	screenPtr = (uint32_t*)modeInfo->physbase;
	// printf("bytesperline : 0x%X\n", (int)bytesperline);
	// printf("screenPtr : 0x%X\n", (int)screenPtr);

	if (!screenPtr && mode >= 0x100)
		return;

	g_mode = mode;

	set_vbe_mode(mode);
}


/* Get SuperVGA information, returning true if VBE found */
int getVbeInfo()
{
	struct bios_regs regs;
	memset(&regs, 0, sizeof(struct bios_regs));

	regs.eax = 0x4F00;
	regs.edi = ((int)vbeInfo);
	regs.es = (int)vbeInfo>>16;
	regs.ebp = 0x5000;
	regs.esp = 0x5000;
	regs.eflags = 0x202;
	regs.ds = 0x18;
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
	// printf("modeInfo %X\n", mode);

	struct bios_regs regs;
	memset(&regs, 0, sizeof(struct bios_regs));

	regs.eax = 0x4F01;
	regs.edi = ((int)modeInfo);
	regs.es = (int)modeInfo>>16;
	regs.ecx = mode;
	regs.ebp = 0x5000;
	regs.esp = 0x5000;
	regs.eflags = 0x202;
	regs.ds = 0x18;
	regs.fs = 0x18;
	regs.gs = 0x18;
	regs.ss = 0x18;

	do_bios_call(&regs, 0x10);

	if (REGX(regs.eax) != 0x004F)
		return 0;

	// printf("regs.eax %X\n", regs.eax);

	attributes = modeInfo->attributes;
	memory_model = modeInfo->memory_model;
	planes = modeInfo->planes;

	// printf("attributes %d  memory_model %d  planes %d\n", attributes, memory_model, planes);

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
		printf("0x%X : %dx%dx%d 0x%X\n", modes[i], inf->Xres,
			inf->Yres, inf->bpp, inf->physbase);
	}
}

static inline void draw_screen();
static inline void display_bg();

int init_graphics(unsigned int x, unsigned int y, unsigned int d)
{
	g_mode = findMode(x, y, d);
	init_vbe_mode(g_mode);

	display_bg();
	draw_screen();

	return (int)screenPtr;
}

static inline void draw_screen()
{
	if (!screenPtr || (get_physaddr(get_pdir(), (void*)VIDEO_MEMORY) != screenPtr))
		return;

	memcpy((void*)VIDEO_MEMORY, (void*)screenBuf, xres * yres * bpp/8);
}

static inline void draw_image(char *data, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	uint32_t k = 0;

	for (uint32_t j = y; j < h + y; j++)
		for (uint32_t i = x; i < w + x; i++) {

			const char b = data[k++];
			const char g = data[k++];
			const char r = data[k++];

			putPixelRGB(i, j, r, g, b);
		}
}

static inline void display_bg()
{
	uint16_t *data = (uint16_t*)bg_bin;

	uint16_t width = data[0];
	uint16_t height = data[1];

	char *rgb_data = (char*)data + 4;

	// Draw background tiles
	for (uint32_t i = 0; i < xres; i += width)
		for (uint32_t j = 0; j < yres; j += height) {
			draw_image(rgb_data, i, j, width, height);
		}
}

static inline void display_image()
{
	const uint32_t x = (xres - img_width) / 2;
	const uint32_t y = (yres - img_height) / 2;

	display_bg();
	draw_image(img_data, x, y, img_width, img_height);
}

// Currently only used for mouse display purposes
void fill_rectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
	if (!screenPtr)
		return;

	(void)w;
	(void)h;
	(void)color;

	// Mouse events may occur from
	// another process with a different memory mapping
	map_video_memory();

	display_image();
	draw_image((char*)cursor_bin, x, y, 16, 24);
	draw_screen();
}

bool display(char *image)
{
	uint32_t size = 0;
	void *data = atomicData(atomicOpen(image), &size);

	if (data == NULL || size <= 4)
		return false;

	img_width = ((uint16_t*)data)[0];
	img_height = ((uint16_t*)data)[1];

	// Check data size
	if (size < (4 + (uint32_t)img_width * img_height * 3))
		return false;

	init_vbe_mode(findMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32));
	map_video_memory();

	img_data = (char*)data + 4;

	display_image();
	draw_screen();

	return true;
}

