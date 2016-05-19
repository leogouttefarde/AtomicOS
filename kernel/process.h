
#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdbool.h>
#include <stdint.h>


void idle();

// Cree le processus idle (pas besoin de stack)
// Il faut l'appeler en premier dans start
bool init_idle();

// Ordonnanceur
void ordonnance();

// Endort un processus
void sleep(uint32_t seconds);

// Affiche l'état des processus
void affiche_etats(void);

// Termine le processus courant
void fin_processus();

/**
 * Donne la priorité newprio au processus identifié par la valeur de pid.
 * Si newprio ou pid invalide, la valeur de retour est strictement négative,
 * sinon elle est égale à l'ancienne priorité du processus pid.
 * pid     : identifiant du processus
 * newprio : nouvelle priorité
 */
int chprio(int pid, int newprio);

/**
 * Si la valeur de pid est invalide, la valeur de retour est strictement
 * négative, sinon elle est égale à la priorité du processus identifié par
 * la valeur pid.
 * pid     : identifiant du processus
 */
int getprio(int pid);

/**
 * Crée un nouveau processus dans l'état activable ou actif selon la priorité
 * choisie. Retourne l'identifiant du processus, ou une valeur strictement
 * négative en cas d'erreur.
 * name  : programme associé au processus
 * ssize : taille utilisable de la pile
 * prio  : priorité du processus
 * arg   : argument passé au programme
 */
int start(const char *name, unsigned long ssize, int prio, void *arg, int (*pt_func)(void*));

/**
 * Le processus appelant est terminé normalement et la valeur retval est passée
 * à son père quand il appelle waitpid. La fonction exit ne retourne jamais à
 * l'appelant.
 */
void _exit(int retval);

/**
 * La primitive kill termine le processus identifié par la valeur pid.
 * Si la valeur de pid est invalide, la valeur de retour est strictement
 * négative. En cas de succès, la valeur de retour est nulle.
 */
int kill(int pid);

/**
 * Si pid est négatif, le processus appelant attend qu'un
 * de ses fils, n'importe lequel, soit terminé et récupère (le cas échéant)
 * sa valeur de retour dans *retvalp, à moins que retvalp soit nul.
 * Cette fonction renvoie une valeur strictement négative si aucun fils
 * n'existe ou sinon le pid de celui dont elle a récupéré la valeur de retour.
 *
 * Si pid est positif, le processus appelant attend que son fils
 * ayant ce pid soit terminé ou tué et récupère sa valeur de retour
 * dans *retvalp, à moins que retvalp soit nul. Cette fonction échoue et
 * renvoie une valeur strictement négative s'il n'existe pas de processus avec
 * ce pid ou si ce n'est pas un fils du processus appelant. En cas de succès,
 * elle retourne la valeur pid.
 * Lorsque la valeur de retour d'un fils est récupérée, celui-ci est détruit,
 * et enlevé de la liste des fils.
 */
int waitpid(int pid, int *retvalp);

/**
 * Renvoie le pid du processus actuel.
 */
int getpid(void);



#endif
