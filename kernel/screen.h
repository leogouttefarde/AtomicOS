
#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <stdint.h>

#define VIDEO_MEMORY 0xB8000
#define NB_COLS 80
#define NB_LIG  25
#define BG_COL  NOIR
#define FG_COL  BLANC

// Renvoie un pointeur sur la case mémoire
// correspondant aux coordonnées fournies
#define ptr_mem(lig, col) ((uint16_t*)(VIDEO_MEMORY + 2 * (lig * NB_COLS + col)))

typedef enum Couleur_ Couleur;


// Ecrit le caractère c aux coordonnées spécifiées, en spécifiant le style.
void ecrit_car_style(uint32_t lig, uint32_t col, char c,
			Couleur couleur, Couleur fond, uint8_t clignotement);

// Ecrit le caractère c aux coordonnées spécifiées avec le style courant
void ecrit_car(uint32_t lig, uint32_t col, char c);

void place_curseur(uint8_t lig, uint8_t col);
void efface_ecran(void);
void traite_car(char c);
void defilement(void);
void console_putbytes(const char *chaine, int32_t taille);

void banner();

uint8_t col_cour();
uint8_t lig_cour();

// Couleurs
enum Couleur_ {

	// Texte et fond
	NOIR = 0,
	BLEU,
	VERT,
	CYAN,
	ROUGE,
	MAGENTA,
	MARRON,
	GRIS,

	// Texte seulement
	GRIS_FONCE,
	BLEU_CLAIR,
	VERT_CLAIR,
	CYAN_CLAIR,
	ROUGE_CLAIR,
	MAGENTA_CLAIR,
	JAUNE,
	BLANC,

	NB_COULS
};


#endif
