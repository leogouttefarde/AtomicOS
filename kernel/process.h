#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdbool.h>
#include <stdint.h>
#include <queue.h>
#include <hash.h>
#include "semaphores.h"

#define PROC_NAME_SIZE 64
#define MAX_NB_PROCS 1000
#define MAX_PRIO 256


// Macros de files
#define cqueue_for_each(proc, head) queue_for_each(proc, head, Process, children)
#define pqueue_for_each(proc, head) queue_for_each(proc, head, Process, queue)
#define pqueue_for_each_prev(proc, head) queue_for_each_prev(proc, head, Process, queue)
#define pqueue_add(proc, head) 							\
	do {												\
		INIT_LINK(&proc->queue);						\
		queue_add(proc, head, Process, queue, prio);	\
	} while (0)

#define IS_USER(arg) ((USERCODE <= (uint32_t)arg) || !arg)
#define IS_USER2(a0, a1) (IS_USER(a0) && IS_USER(a1))

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
	WAITPID,
	BLOCKEDSEMA,
	WAITMSG,
	WAITIO
};

enum SavedRegisters {
	EBX,
	ESP,
	EBP,
	ESI,
	EDI,
	ESP0, // 4(TSS)
	NB_REGS
};

enum PagingFlags {
	P_PRESENT = 1,
	P_RW = 2,
	P_USERSUP = 4
};

/* Processus */
typedef struct Process_ {
	int pid;
	int ppid;

	union {
		int retval;	// Valeur de retour de ZOMBIE
		int waitpid;	// pid à attendre en WAITPID
	} s;

	link head_child;
	link children;
	link queue;

	int message;
	int *msg_counter;
	bool has_msg_error;
	link *blocked_queue;
	link msg_queue;
	link sema_queue;

	int prio;

	struct Process_ *sibling;
	char name[PROC_NAME_SIZE];
	enum State state;
	uint32_t regs[NB_REGS];
	uint32_t cpages;
	uint32_t spages;
	uint32_t *stack;
	uint32_t *kstack;
	unsigned long ssize;
	uint32_t wake;

	uint32_t *pdir;
	uint32_t *ptable;

	hash_t shmem;
	uint32_t shm_idx;
	
	uint8_t code_reveil; /*indique si le processus a été placé dans la 
			       liste des activable par un sdelete (code 3), par
			       un reset (code 4), ou par un signal (code 0)*/

	semaphore *sema;
		       
} Process;


void idle();

void wait_clock(unsigned long clock);


// A appeler en premier dans kernel_start
bool init_process();

// Ordonnanceur
void ordonnance();

//Bloque un processus par le biais d'un sémaphore
void bloque_sema();

//Debloque un processus bloqué par un sémaphore
void debloque_sema(Process *p, uint8_t code);

//Bloque un processus en attendant une entrée/sortie
void bloque_io();

//Débloque un processus en attente d'entrée/sortie
void debloque_io();

// Endort un processus
void sleep(uint32_t seconds);

// Affiche l'état des processus
void affiche_etats(void);

// Termine le processus courant
void fin_processus();

unsigned int get_code_reveil ();

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
int start(const char *name, unsigned long ssize, int prio, void *arg);

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


__inline__ static void tlb_flush()
{
	__asm__ __volatile__(
	"movl %cr3,%eax\n"
	"movl %eax,%cr3");
}


Process *get_cur_proc();

//Ajout d'un processus dans la liste des processus activable
void add_proc_activable(Process *proc);

void sleep_clock(uint32_t clocks);

void launch_new_shell();
void abort_shell_wait();
void setup_exceptions();

#endif
