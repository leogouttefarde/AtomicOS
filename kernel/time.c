
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <cpu.h>
#include <segment.h>
#include "screen.h"
#include "time.h"
#include "process.h"

#define IDT_ADDRESS 0x1000
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

// Enregistre un traitant d'interruption
static inline void init_traitant_IT(int32_t num_IT, void (*traitant)(void))
{
	const uint64_t *IDT = (uint64_t*)IDT_ADDRESS;
	uint32_t *entree_IT = (uint32_t*)(IDT + num_IT);

	*entree_IT = KERNEL_CS << 16 | (uint16_t)(uint32_t)traitant;
	*(entree_IT + 1) = ((uint32_t)traitant & 0xFFFF0000) | 0x8E00;
}

// Fonction de masquage générique d'un IRQ
static inline void masque_IRQ(uint8_t num_IRQ, bool masque)
{
	if (num_IRQ < 8 && masque <= 1) {

		// Récupération des masques IRQ
		uint8_t sortie = inb(0x21);

		// On efface l'ancien masque de l'IRQ
		sortie &= ~(1 << num_IRQ);

		// On écrit le nouveau
		sortie |= masque << num_IRQ;

		outb(sortie, 0x21);
	}
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


