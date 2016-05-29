
#ifndef __SYSAPI_CUSTOM_H__
#define __SYSAPI_CUSTOM_H__

#include <sysapi.h>

// Colors
typedef enum Color_ {

	// Text & Background colors
	BLACK = 0,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	GRAY,

	// Text only colors
	DARK_GRAY,
	LIGHT_BLUE,
	LIGHT_GREEN,
	LIGHT_CYAN,
	LIGHT_RED,
	LIGHT_MAGENTA,
	YELLOW,
	WHITE,

	NB_COLORS
} Color;

void cons_set_bg_color(Color color);
void cons_set_fg_color(Color color);
void cons_reset_color();

void affiche_etats();
void reboot();
void sleep(unsigned long seconds);


#endif
