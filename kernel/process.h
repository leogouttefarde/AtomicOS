
#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdbool.h>
#include <stdint.h>


void idle();

// Cree le processus idle (pas besoin de stack)
// Il faut l'appeler en premier dans start
bool init_idle();

// Cree un processus générique
int32_t cree_processus(void (*code)(void), char *nom);

// Ordonnanceur
void ordonnance();

// Endort un processus
void dors(uint32_t nbr_secs);

// Affiche l'état des processus
void affiche_etats(void);

// Termine le processus courant
void fin_processus();


#endif
