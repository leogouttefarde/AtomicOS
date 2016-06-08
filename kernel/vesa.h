
#ifndef __VESA_H__
#define __VESA_H__

#include <stdint.h>
#include <stdbool.h>

#define SCREEN_WIDTH 1366
#define SCREEN_HEIGHT 768
// #define SCREEN_WIDTH 1680
// #define SCREEN_HEIGHT 1050

void set_vesa();
uint16_t findMode(int x, int y, int d);
void set_vbe_mode(int mode);
void init_vbe_mode(int mode);
int init_graphics(unsigned int x, unsigned int y, unsigned int d);
void list_modes(int min, int max);
void fill_rectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
int getModeInfo(int mode);
bool is_console_mode();
void set_console_mode();
bool display(char *image);

uint32_t get_screen_width();
uint32_t get_screen_height();

#endif
