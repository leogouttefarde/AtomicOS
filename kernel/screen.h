
#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <stdint.h>

#define CONSOLE_MEMORY 0xB8000
#define NB_COLS 80
#define NB_LIG  25
#define BG_COL  BLACK
#define FG_COL  WHITE

// Renvoie un pointeur sur la case mémoire
// correspondant aux coordonnées fournies
#define ptr_mem(lig, col) ((uint16_t*)(CONSOLE_MEMORY + 2 * (lig * NB_COLS + col)))

typedef enum Color_ Color;


// Ecrit le caractère c aux coordonnées spécifiées, en spécifiant le style.
void ecrit_car_style(uint32_t lig, uint32_t col, char c,
			Color color, Color fond, uint8_t clignotement);

// Ecrit le caractère c aux coordonnées spécifiées avec le style courant
void ecrit_car(uint32_t lig, uint32_t col, char c);

void place_curseur(uint8_t lig, uint8_t col);
void efface_ecran(void);
void traite_car(char c);
void scrollup(uint8_t lines);
void console_putbytes(const char *chaine, int32_t taille);

void banner();

uint8_t col_cour();
uint8_t lig_cour();

void set_bg_color(Color color);
void set_fg_color(Color color);
void reset_color();

//Renvoyer la largeur de l'écran
int get_Width();

//Renvoyer la hauteur de l'écran
int get_Height();

// Couleurs
enum Color_ {

	// Texte et fond
	BLACK = 0,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	GRAY,

	// Texte seulement
	DARK_GRAY,
	LIGHT_BLUE,
	LIGHT_GREEN,
	LIGHT_CYAN,
	LIGHT_RED,
	LIGHT_MAGENTA,
	YELLOW,
	WHITE,

	NB_COLORS
};


#endif
