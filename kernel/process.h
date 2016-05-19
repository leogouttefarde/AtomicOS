
#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdbool.h>
#include <stdint.h>
#include <queue.h>

#define PROC_NAME_SIZE 16
#define MAX_NB_PROCS 100
#define MAX_PRIO 256

/*
 * Etats de gestion
 * des processus
 */
enum State {
	CHOSEN,
	ACTIVABLE,
	ASLEEP,
	DYING,
	ZOMBIE,
	WAITPID	// Etat d'attente d'un fils
};

/* Liste de processus */
typedef struct ListProc_ {
	struct Process_ *head;
	struct Process_ *tail;
} ListProc;

/* Processus */
typedef struct Process_ {
	int pid;
	int ppid;

	union {
		int retval;		// Valeur de retour de ZOMBIE
		int waitpid;	// pid à attendre en WAITPID
	} s;

	ListProc children;
	struct Process_ *sibling;
	char name[PROC_NAME_SIZE];
	enum State state;
	int regs[5];
	int *stack;
	unsigned long ssize;
	uint32_t wake;

	union {
		struct Process_ *next;
		link lnext;
	} u;

	int prio;
} Process;


void idle();

// Cree le processus idle (pas besoin de stack)
// Il faut l'appeler en premier dans start
bool init_idle();

// Ordonnanceur
void ordonnance();

// Endort un processus
void dors(uint32_t nbr_secs);

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
void exit(int retval);

/**
 * La primitive kill termine le processus identifié par la valeur pid.
 * Si la valeur de pid est invalide, la valeur de retour est strictement
 * négative. En cas de succès, la valeur de retour est nulle.
 */
int kill(int pid);



#endif
