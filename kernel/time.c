
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <cpu.h>
#include <segment.h>
#include "screen.h"
#include "time.h"
#include "process.h"
#include "interruptions.h"

#define QUARTZ 0x1234DD


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
	const uint16_t freq_sig = QUARTZ / freq;

	outb(0x34, 0x43);
	outb(freq_sig, 0x40);
	outb(freq_sig >> 8, 0x40);
}

/**
 * Initialise le module.
 */
void init_time()
{
	init_traitant_IT(32, traitant_IT_32);
	init_freq(SCHEDFREQ);
	masque_IRQ(0, false);
}

/**
 * Retourne le nombre de secondes écoulées depuis le démarrage du noyau.
 */
uint32_t get_time()
{
	return g_secs;
}

// Fonction du traitant de l'interruption horloge
void tic_PIT(void)
{
	// Acquittement de l'interruption
	outb(0x20, 0x20);

	// Incrémentation des secondes
	if (!(++g_tics % SCHEDFREQ)) {
		g_secs++;
	}

	char temps[9];
	const uint32_t mns = g_secs / 60;
	const uint32_t hrs = mns / 60;

	sprintf(temps, "%02d:%02d:%02d", hrs, mns % 60, g_secs % 60);
	affiche_temps(temps);

	ordonnance();
}

/**
 * Retourne dans *quartz la fréquence du quartz du système et
 * dans *ticks le nombre d'oscillations du quartz entre chaque interruption.
 */
void clock_settings(unsigned long *quartz, unsigned long *ticks)
{
	*quartz = QUARTZ;
	*ticks = QUARTZ / SCHEDFREQ;
}

/**
 * Retourne le nombre d'interruptions d'horloge depuis le démarrage du noyau.
 */
unsigned long current_clock()
{
	return g_tics;
}

