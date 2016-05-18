
#include <stdio.h>
#include <cpu.h>
#include <stddef.h>
#include <string.h>
#include <queue.h>
#include "process.h"
#include "time.h"
#include "mem.h"

#define PROC_NAME_SIZE 16
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
	DYING,
	ZOMBIE,
	STANDBY	// Etat d'attente d'un fils
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
	int waitpid;	// pid à attendre si STANDBY
	ListProc children;
	struct Process_ *sibling;
	char name[PROC_NAME_SIZE];
	enum state state;
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


// Nombre de processus créés depuis le début
static int32_t nb_procs = -1;

// Processus
static Process* procs[MAX_NB_PROCS];
static Process *cur_proc = NULL;

// Listes de gestion des processus
static link head_act = LIST_HEAD_INIT(head_act);
// static ListProc list_act   = { NULL, NULL };
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
				break;

			default:
				break;
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
		list->head = head->u.next;

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

	proc->u.next = NULL;

	if (list->tail) {
		list->tail->u.next = proc;
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

	proc->u.next = list->head;

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
		Process *next = pos->u.next;

		if (next) {
			pos->u.next = proc;
			proc->u.next = next;
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
	Process *next;

	// Libération des processus un à un
	while (die) {
		next = die->u.next;

		if (die->stack)
			mem_free(die->stack, die->ssize);

		procs[die->pid] = NULL;
		mem_free(die, sizeof(Process));

		die = next;
	}

	memset(&list_dead, 0, sizeof(list_dead));
}

// Réveille les processus endormis dont l'heure de réveil est atteinte ou dépassée
static inline void wake_procs()
{
	Process *it = list_sleep.head;
	Process *next;

	while (it && (it->wake <= get_temps())) {
		next = it->u.next;
		pop_head(&list_sleep);

		it->state = ACTIVABLE;
		// add_tail(&list_act, it);
		INIT_LINK(&it->u.lnext);
		queue_add(it, &head_act, Process, u.lnext, prio);
		//printf("ajout %s aux actifs, wake = %d, t = %d\n", it->name, it->wake, get_temps());

		it = next;
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
			prev->state = ACTIVABLE;
			// add_tail(&list_act, prev);
			INIT_LINK(&prev->u.lnext);
			queue_add(prev, &head_act, Process, u.lnext, prio);
		}

		// On extrait le processus suivant
		// cur_proc = pop_head(&list_act);
		cur_proc = queue_out(&head_act, Process, u.lnext);

		// On passe au processus voulu
		if (cur_proc) {
			cur_proc->state = CHOSEN;

			// Changement de processus courant si nécessaire.
			if (cur_proc != prev) {
				// affiche_etats();
				ctx_sw(prev->regs, cur_proc->regs);
			}
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
		it = it->u.next;
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

		proc->prio = 0;
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

/**
 * Crée un nouveau processus dans l'état activable ou actif selon la priorité
 * choisie. Retourne l'identifiant du processus, ou une valeur strictement
 * négative en cas d'erreur.
 * name  : programme associé au processus
 * ssize : taille utilisable de la pile
 * prio  : priorité du processus
 * arg   : argument passé au programme
 */
int start(const char *name, unsigned long ssize, int prio, void *arg, int (*pt_func)(void*))
{
	int32_t pid = -1;

	if (nb_procs < MAX_NB_PROCS && name != NULL
		&& 0 < prio && prio <= MAX_PRIO) {
		Process *proc = mem_alloc(sizeof(Process));

		if (proc != NULL) {

			uint32_t stack_size = ssize/4 + 3;

			// Si le nombre n'est pas multiple
			// de sizeof(int), on ajoute une case
			if (ssize % sizeof(int))
				stack_size++;

			proc->ssize = stack_size * sizeof(int);
			proc->stack = mem_alloc(proc->ssize);

			if (proc->stack != NULL) {

				//printf("[temps = %u] creation processus %s pid = %i\n", nbr_secondes(), name, pid);
				proc->prio = prio;
				proc->ppid = getpid();
				pid = proc->pid = nb_procs++;

				strncpy(proc->name, name, PROC_NAME_SIZE);
				proc->name[PROC_NAME_SIZE-1] = 0;

				// On met l'adresse de terminaison du processus en sommet de pile
				// pour permettre son auto-destruction
				proc->stack[stack_size - 1] = (int)arg;
				proc->stack[stack_size - 2] = (int)fin_processus;
				proc->stack[stack_size - 3] = (int)pt_func;

				// On initialise esp sur le sommet de pile désiré
				proc->regs[1] = (int)&proc->stack[stack_size - 3];

				procs[pid] = proc;

				// Ajout aux activables
				proc->state = ACTIVABLE;
				// add_tail(&list_act, proc);
				INIT_LINK(&proc->u.lnext);
				queue_add(proc, &head_act, Process, u.lnext, prio);

				if (prio > cur_proc->prio) {
					ordonnance();
				}
			}
			else
				mem_free(proc, sizeof(Process));
		}
	}

	return pid;
}

