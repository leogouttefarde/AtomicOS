
#include "screen.h"
#include "vesa.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <cpu.h>

// Constantes d'affichage
static Color couleur_texte = FG_COL;
static Color couleur_fond = BG_COL;
static const uint8_t clignotement = 0;

// Variables du curseur
static uint8_t cur_lig = 0;
static uint8_t cur_col = 0;

uint8_t col_cour()
{
        return cur_col;
}

uint8_t lig_cour()
{
        return cur_lig;
}

// Ecrit le caractère c aux coordonnées spécifiées, en spécifiant le style.
void ecrit_car_style(uint32_t lig, uint32_t col, char c,
			Color couleur, Color fond, uint8_t clignotement)
{
	if (lig < NB_LIG && col < NB_COLS && is_console_mode()) {
		uint16_t *block = ptr_mem(lig, col);

		uint8_t style = (((clignotement & 1) << 1)
						| (fond & 0x7)) << 4
						| (couleur & 0xF);


		*block = style << 8 | c;
	}
}

// Ecrit le caractère c aux coordonnées spécifiées avec le style courant
void ecrit_car(uint32_t lig, uint32_t col, char c)
{
	ecrit_car_style(lig, col, c, couleur_texte, couleur_fond, clignotement);
}

void place_curseur(uint8_t lig, uint8_t col)
{
	if (lig < NB_LIG && col < NB_COLS) {
		const uint16_t pos = col + lig * NB_COLS;

		const uint16_t command_port = 0x3D4;
		const uint16_t data_port = 0x3D5;

		outb(0x0F, command_port);
		outb(pos, data_port);

		outb(0x0E, command_port);
		outb(pos >> 8, data_port);


		cur_lig = lig;
		cur_col = col;
	}
}

void efface_ecran(void)
{
	for (uint8_t i = 0; i < NB_LIG; i++)
		for (uint8_t j = 0; j < NB_COLS; j++)
			ecrit_car(i, j, 0);

	place_curseur(0, 0);
}

static inline void finir_ligne()
{
	cur_col = 0;

	if (cur_lig == (NB_LIG - 1))
		defilement();

	else
		cur_lig++;
}

void traite_car(char c)
{
	bool saut_ligne = false;

	if (32 <= c && c <= 126)
		ecrit_car(cur_lig, cur_col++, c);

	else {
		switch (c) {
			case '\b':
				if (cur_col)
					--cur_col;
				break;

			case '\t':
				cur_col += 8 - (cur_col % 8);
				break;

			case '\n':
				if (!saut_ligne)
					finir_ligne();
				break;

			case '\f':
				efface_ecran();
				break;

			case '\r':
				cur_col = 0;
				break;
		}
	}

        if (cur_col >= NB_COLS) {
		finir_ligne();
		saut_ligne = true;
	}


	place_curseur(cur_lig, cur_col);
}

void defilement(void)
{
	uint16_t *mem = (uint16_t*)VIDEO_MEMORY;

	memmove(mem, mem + NB_COLS, sizeof(uint16_t)*NB_COLS*(NB_LIG-1));

	for (uint8_t j = 0; j < NB_COLS; j++)
		ecrit_car(NB_LIG - 1, j, 0);
}

void console_putbytes(const char *chaine, int32_t taille)
{
	for (int32_t i = 0; i < taille; i++)
		traite_car(chaine[i]);
}

void reset_color()
{
	couleur_texte = FG_COL;
	couleur_fond = BG_COL;
}

void set_bg_color(Color color)
{
	couleur_fond = color;
}

void set_fg_color(Color color)
{
	couleur_texte = color;
}

//Renvoyer la largeur de l'écran
int get_Width()
{
	return NB_COLS;
}

//Renvoyer la hauteur de l'écran
int get_Height()
{
	return NB_LIG;
}

void banner()
{
	const char *name =
///*
"\n      _____     __                    .__         /\\________     _________\n"\
"     /  _  \\  _/  |_   ____    _____  |__|  ____  )/\\_____  \\   /   _____/\n"\
"    /  /_\\  \\ \\   __\\ /  _ \\  /     \\ |  |_/ ___\\    /   |   \\  \\_____  \\ \n"\
"   /    |    \\ |  |  (  <_> )|  Y Y  \\|  |\\  \\___   /    |    \\ /        \\\n"\
"   \\____|__  / |__|   \\____/ |__|_|  /|__| \\___  >  \\_______  //_______  /\n"\
"           \\/                      \\/          \\/           \\/         \\/ \n\n"
//*/
/*
" _______  _______  _______  __   __  ___   _______  __   _______  _______ \n"\
"|   _   ||       ||       ||  |_|  ||   | |       ||  | |       ||       |\n"\
"|  |_|  ||_     _||   _   ||       ||   | |       ||__| |   _   ||  _____|\n"\
"|       |  |   |  |  | |  ||       ||   | |       |     |  | |  || |_____ \n"\
"|       |  |   |  |  |_|  ||       ||   | |      _|     |  |_|  ||_____  |\n"\
"|   _   |  |   |  |       || ||_|| ||   | |     |_      |       | _____| |\n"\
"|__| |__|  |___|  |_______||_|   |_||___| |_______|     |_______||_______|\n\n"
*/
/*
"                                 _               \n"\
" _____  _                _      | | _____  _____ \n"\
"|  _  || |_  ___  _____ |_| ___ |_||     ||   __|\n"\
"|     ||  _|| . ||     || ||  _|   |  |  ||__   |\n"\
"|__|__||_|  |___||_|_|_||_||___|   |_____||_____|\n"\
"\n"
*/
;

	printf("\f");

	// set_fg_color(LIGHT_CYAN);
	set_fg_color(LIGHT_BLUE);

	printf("%s",name);
	reset_color();

	printf("        Welcome to the ultimate last resort system.\n\n");
}


