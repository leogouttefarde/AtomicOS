
#include <stdio.h>
#include <cpu.h>
#include <stddef.h>
#include <string.h>
#include "process.h"
#include "time.h"
#include "mem.h"

#define PROC_NAME_SIZE 16
#define STACK_SIZE 1024
#define MAX_NB_PROCS 100
#define MAX_PRIO 256

/*
 * Etats de gestion
 * des processus
 */
enum state {
	CHOSEN,
	ACTIVABLE,
	ASLEEP,
	DYING
};

/* Processus */
typedef struct process {
	int pid;
	int ppid;
	char name[PROC_NAME_SIZE];
	enum state state;
	int regs[5];
	int *stack;
	unsigned long ssize;
	struct process *suiv;
	uint32_t wake;
	int prio;
} Process;

/* Liste de processus */
typedef struct ListProc_ {
	Process *head;
	Process *tail;
} ListProc;


// Nombre de processus créés depuis le début
static int32_t nb_procs = -1;

// Processus
static Process* procs[MAX_NB_PROCS];
static Process *cur_proc = NULL;

// Listes de gestion des processus
static ListProc list_act   = { NULL, NULL };
static ListProc list_sleep = { NULL, NULL };
static ListProc list_dead  = { NULL, NULL };


void ctx_sw(int32_t *old_ctx, int32_t *new_ctx);


// Affiche l'état des processus
void affiche_etats(void)
{
	Process *proc;

	printf("Affichage des processus\n");

	for (uint32_t i = 0; i < MAX_NB_PROCS; i++) {
		proc = procs[i];

		if (proc != NULL) {
			char *state = "NONE";

			switch (proc->state) {
			case CHOSEN:
				state = "CHOSEN";
				break;

			case ACTIVABLE:
				state = "ACTIVABLE";
				break;

			case ASLEEP:
				state = "ASLEEP";
				break;

			case DYING:
				state = "DYING";
			}

			printf("    processus numero %d : %s dans l'etat %s\n", proc->pid, proc->name, state);
		}
	}
}

/* Extrait un élément en début de liste */
static inline Process* pop_head(ListProc *list)
{
	if (list == NULL)
		return NULL;

	Process *head = list->head;

	if (head) {
		list->head = head->suiv;

		if (list->tail == head) {
			list->head = NULL;
			list->tail = NULL;
		}
	}

	return head;
}

/* Insère en fin de liste */
static inline void add_tail(ListProc *list, Process *proc)
{
	if (!list || !proc) return;

	proc->suiv = NULL;

	if (list->tail) {
		list->tail->suiv = proc;
		list->tail = proc;
	}
	else {
		list->head = proc;
		list->tail = proc;
	}
}

/* Insère en tête de liste */
static inline void add_head(ListProc *list, Process *proc)
{
	if (!list || !proc) return;

	proc->suiv = list->head;

	if (!list->head)
		list->tail = proc;

	list->head = proc;
}

/* Insère proc dans list après l'élément pos.
 * Si pos == NULL, insère en tête */
static inline void insert(ListProc *list, Process *pos, Process *proc)
{
	if (!list || !proc)
		return;

	if (pos == NULL)
		add_head(list, proc);

	else {
		Process *suiv = pos->suiv;

		if (suiv) {
			pos->suiv = proc;
			proc->suiv = suiv;
		}
		else
			add_tail(list, proc);
	}
}

static inline char *mon_nom()
{
	if (!cur_proc)    return "";

	return cur_proc->name;
}

int getpid()
{
	if (!cur_proc)    return -1;

	return cur_proc->pid;
}

static inline int32_t nbr_secondes()
{
	if (!cur_proc)    return -1;

	return cur_proc->wake;
}

// Détruit les processus mourants
static inline void kill_procs()
{
	Process *die = list_dead.head;
	Process *suiv;

	// Libération des processus un à un
	while (die) {
		suiv = die->suiv;

		if (die->stack)
			mem_free(die->stack, die->ssize);

		procs[die->pid] = NULL;
		mem_free(die, sizeof(Process));

		die = suiv;
	}

	memset(&list_dead, 0, sizeof(list_dead));
}

// Réveille les processus endormis dont l'heure de réveil est atteinte ou dépassée
static inline void wake_procs()
{
	Process *it = list_sleep.head;
	Process *suiv;

	while (it && (it->wake <= get_temps())) {
		suiv = it->suiv;
		pop_head(&list_sleep);

		add_tail(&list_act, it);
		it->state = ACTIVABLE;
		//printf("ajout %s aux actifs, wake = %d, t = %d\n", it->name, it->wake, get_temps());

		it = suiv;
	}
}

// Ordonnanceur
void ordonnance()
{
	// On détruit les processus mourants
	kill_procs();

	// On réveille les processus qui le doivent
	wake_procs();

	// On passe au processus suivant
	if (cur_proc) {
		Process *prev = cur_proc;

		// On ajoute le processus courant aux mourants
		// pour le tuer au prochain ordonnancement
		if (prev->state == DYING)
			add_tail(&list_dead, prev);

		// Si processus ni tué ni endormi,
		// on le remet avec les activables
		else if (prev->state != ASLEEP) {
			add_tail(&list_act, prev);
			prev->state = ACTIVABLE;
		}

		// On extrait le processus suivant
		cur_proc = pop_head(&list_act);

		// On passe au processus voulu
		if (cur_proc) {
			cur_proc->state = CHOSEN;

			// Changement de processus courant si nécessaire.
			if (cur_proc != prev)
				ctx_sw(prev->regs, cur_proc->regs);
		}
	}
}

// Endort un processus
void dors(uint32_t nbr_secs)
{
	Process *proc_sleep = cur_proc;

	// Si pas de processus à endormir différent de idle, abandon
	if (!proc_sleep || !proc_sleep->pid)
		return;

	//printf("ajout de %s aux dormants pour %d secondes\n", mon_nom(), nbr_secs);

	proc_sleep->state = ASLEEP;
	proc_sleep->wake = nbr_secs + get_temps();

	Process *it = list_sleep.head;
	Process *prec = NULL;
	bool not_done = true;

	while (it && not_done) {
		if (it->wake > proc_sleep->wake) {
			insert(&list_sleep, prec, proc_sleep);
			not_done = false;
		}

		prec = it;
		it = it->suiv;
	}

	// Si le processus n'a toujours pas
	// été inséré, on l'insère en fin de liste
	if (not_done) {
		add_tail(&list_sleep, proc_sleep);
	}

	ordonnance();
}

void idle(void)
{
	sti();

	for (;;) {
		hlt();
		// affiche_etats();
	}
}

// Termine le processus courant
void fin_processus()
{
	// Si pas de processus à tuer différent de idle, abandon
	if (cur_proc == NULL || !cur_proc->pid)
		return;

	//printf("fin processus %s pid = %i\n", mon_nom(), mon_pid());
	cur_proc->state = DYING;

	ordonnance();
}

// Cree le processus idle (pas besoin de stack)
// Il faut l'appeler en premier dans start
bool init_idle()
{
	Process *proc = mem_alloc(sizeof(Process));

	if (proc != NULL) {
		const int32_t pid = 0;

		proc->ppid = pid;
		proc->pid = pid;
		strcpy(proc->name, "idle");

		procs[pid] = proc;

		// idle est le processus initial
		cur_proc = proc;
		proc->state = CHOSEN;

		nb_procs = 1;
	}

	return (proc != NULL);
}

// Cree un processus générique
int start(const char *name, unsigned long ssize, int prio, void *arg, int (*pt_func)(void*))
{
	int32_t pid = -1;

	if (nb_procs < MAX_NB_PROCS) {
		Process *proc = mem_alloc(sizeof(Process));

		if (proc != NULL) {
			proc->ssize = ssize + 3 * sizeof(int);
			proc->stack = mem_alloc(proc->ssize);

			if (proc->stack != NULL) {

				//printf("[temps = %u] creation processus %s pid = %i\n", nbr_secondes(), name, pid);
				proc->prio = prio;
				proc->ppid = getpid();
				pid = proc->pid = nb_procs++;
				strncpy(proc->name, name, PROC_NAME_SIZE);

				// On met l'adresse de terminaison du processus en sommet de pile
				// pour permettre son auto-destruction
				proc->stack[STACK_SIZE - 1] = (int32_t)arg;
				proc->stack[STACK_SIZE - 2] = (int32_t)fin_processus;
				proc->stack[STACK_SIZE - 3] = (int32_t)pt_func;

				// On initialise esp sur le sommet de pile désiré
				proc->regs[1] = (int32_t)&proc->stack[STACK_SIZE - 3];

				procs[pid] = proc;

				// Ajout aux activables
				proc->state = ACTIVABLE;
				add_tail(&list_act, proc);
			}
			else
				mem_free(proc, sizeof(Process));
		}
	}

	return pid;
}

