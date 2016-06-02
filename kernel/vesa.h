
#ifndef __VESA_H__
#define __VESA_H__

#include <stdint.h>

void set_vesa();
uint16_t findMode(int x, int y, int d);
void setVBEMode(int mode);
int initGraphics(unsigned int x, unsigned int y, unsigned int d);
// void putPixel(int x, int y, uint32_t color);
void list_modes(int min, int max);
void fill_rectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

#endif
