#include <stdint.h>
#include <cpu.h>
#include <stdbool.h>
#include <segment.h>

#define IDT_ADDRESS 0x1000

static inline void init_traitant_IT_generic(int32_t num_IT,
	int traitant, uint16_t flags)
{
	const uint64_t *IDT = (uint64_t*)IDT_ADDRESS;
	uint32_t *entree_IT = (uint32_t*)(IDT + num_IT);

	*entree_IT = KERNEL_CS << 16 | (uint16_t)traitant;
	*(entree_IT + 1) = ((uint32_t)traitant & 0xFFFF0000) | flags;
}

// Enregistre un traitant d'interruption
void init_traitant_IT(int32_t num_IT, int traitant)
{
	init_traitant_IT_generic(num_IT, traitant, 0x8E00);
}

void init_traitant_IT_user(int32_t num_IT, int traitant)
{
	init_traitant_IT_generic(num_IT, traitant, 0xEE00);
}

// Fonction de masquage générique d'un IRQ
void masque_IRQ(uint8_t num_IRQ, bool masque)
{
	masque &= 1;

	if (num_IRQ < 8) {

		// Récupération des masques IRQ
		uint8_t sortie = inb(0x21);

		// On efface l'ancien masque de l'IRQ
		sortie &= ~(1 << num_IRQ);

		// On écrit le nouveau
		sortie |= masque << num_IRQ;

		outb(sortie, 0x21);
	}

	else if (num_IRQ < 16) {

		num_IRQ -= 8;

		// Récupération des masques IRQ
		uint8_t sortie = inb(0xA0);

		// On efface l'ancien masque de l'IRQ
		sortie &= ~(1 << num_IRQ);

		// On écrit le nouveau
		sortie |= masque << num_IRQ;

		outb(sortie, 0xA1);
	}
}
