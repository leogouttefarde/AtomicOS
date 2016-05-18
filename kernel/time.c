
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <cpu.h>
#include <segment.h>
#include "screen.h"
#include "time.h"
#include "process.h"
#include "interruptions.h"

#define SCHEDFREQ 50


// Déclaration du traitant asm
void traitant_IT_32();

// Variables internes
static uint32_t g_tics = 0;
static uint32_t g_secs = 0;


// Affiche le temps en haut à droite
static inline void affiche_temps(char *chaine)
{
	const uint8_t taille = strlen(chaine);
	const uint8_t lig = 0;
	uint8_t col = NB_COLS - taille;

	for (uint32_t i = 0; i < taille; i++)
		ecrit_car(lig, col++, chaine[i]);
}

// Initialise la fréquence d'horloge
static inline void init_freq(const uint32_t freq)
{
	const uint16_t freq_sig = 0x1234DD / freq;

	outb(0x34, 0x43);
	outb(freq_sig, 0x40);
	outb(freq_sig >> 8, 0x40);
}


// Initialise le module
void init_temps()
{
	init_traitant_IT(32, traitant_IT_32);
	init_freq(SCHEDFREQ);
	masque_IRQ(0, false);
}

// Indique le temps depuis le début en secondes
uint32_t get_temps()
{
	return g_secs;
}

// Fonction du traitant de l'interruption horloge
void tic_PIT(void)
{
	// Acquittement de l'interruption
	outb(0x20, 0x20);

	// Incrémentation des secondes
	if (++g_tics == SCHEDFREQ) {
		g_secs++;
		g_tics = 0;
	}

	char temps[9];
	const uint32_t mns = g_secs / 60;
	const uint32_t hrs = mns / 60;

	sprintf(temps, "%02d:%02d:%02d", hrs, mns % 60, g_secs % 60);
	affiche_temps(temps);

	ordonnance();
}


