
#ifndef __INTERRUPTIONS_H__
#define __INTERRUPTIONS_H__

#include <stdbool.h>
#include <stdint.h>

void masque_IRQ(uint8_t num_IRQ, bool masque);
void init_traitant_IT(int32_t num_IT, int traitant);
void init_traitant_IT_user(int32_t num_IT, int traitant);

#endif
