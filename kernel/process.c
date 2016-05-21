
#include <stdio.h>
#include <cpu.h>
#include <stddef.h>
#include <string.h>
#include "process.h"
#include "time.h"
#include "mem.h"
#include "userspace_apps.h"

#define PAGESIZE 0x1000//==(4*1024)

// Nombre de processus créés depuis le début
static int32_t nb_procs = -1;

// Processus
static Process* procs[MAX_NB_PROCS];
static Process *cur_proc = NULL;

// Listes de gestion des processus
static link head_act = LIST_HEAD_INIT(head_act);
static link head_sleep = LIST_HEAD_INIT(head_sleep);
static link head_dead = LIST_HEAD_INIT(head_dead);


void ctx_sw(int32_t *old_ctx, int32_t *new_ctx, int32_t **old_cr3, int32_t *new_cr3);


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

static inline char *mon_nom()
{
	if (!cur_proc)    return "";

	return cur_proc->name;
}

/**
 * Renvoie le pid du processus actuel.
 */
int getpid(void)
{
	// Cas impossible mais géré tout de même
	if (cur_proc == NULL)
		return -1;

	return cur_proc->pid;
}

static inline int32_t nbr_secondes()
{
	if (!cur_proc)    return -1;

	return cur_proc->wake;
}

static inline bool is_valid_prio(int prio)
{
	if (0 < prio && prio <= MAX_PRIO)
		return true;

	return false;
}

static inline void free_process(Process *proc)
{
	if (proc == NULL)
		return;

	if (proc->stack != NULL)
		mem_free_nolength(proc->stack);

	procs[proc->pid] = NULL;
	mem_free_nolength(proc);
}

// Détruit les processus mourants
static inline void kill_procs()
{
	Process *proc = NULL;
	Process *die = NULL;

	queue_for_each(proc, &head_dead, Process, queue) {
		free_process(die);
		die = proc;
	}

	free_process(die);

	// Reset de la liste des mourants
	INIT_LIST_HEAD(&head_dead);
}

static inline void wake_process(Process **psleep)
{
	Process *proc = *psleep;

	if (proc != NULL) {
		*psleep = NULL;
		queue_del(proc, queue);

		proc->state = ACTIVABLE;
		pqueue_add(proc, &head_act);
	}
}

// Réveille les processus endormis dont l'heure de réveil est atteinte ou dépassée
static inline void wake_procs()
{
	Process *it, *wakeup = NULL;

	queue_for_each(it, &head_sleep, Process, queue) {

		wake_process(&wakeup);

		if (it->wake > current_clock())
			break;

		wakeup = it;
	}

	wake_process(&wakeup);
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
		if (prev->state == DYING) {
			pqueue_add(prev, &head_dead);
		}

		// Si l'état du processus reste inchangé,
		// on le remet avec les activables
		else if (prev->state == CHOSEN) {
			prev->state = ACTIVABLE;
			pqueue_add(prev, &head_act);
		}

		// On extrait le processus suivant
		cur_proc = queue_out(&head_act, Process, queue);
		assert(cur_proc);

		// On passe au processus voulu
		if (cur_proc) {
			cur_proc->state = CHOSEN;

			// Changement de processus courant si nécessaire.
			if (cur_proc != prev) {
				// affiche_etats();
				ctx_sw(prev->regs, cur_proc->regs, &prev->pdir, cur_proc->pdir);
			}
		}
	}
}

/**
 * Passe le processus dans l'état endormi jusqu'à ce que l'interruption
 * dont le numéro est passé en paramètre soit passée.
 */
void wait_clock(unsigned long clock)
{
	Process *proc_sleep = cur_proc;

	// Si pas de processus à endormir différent de idle, abandon
	if (!proc_sleep || !proc_sleep->pid)
		return;

	//printf("ajout de %s aux dormants pour %d clocks\n", mon_nom(), clock);

	proc_sleep->state = ASLEEP;
	proc_sleep->wake = clock;

	// Insertion triée du proc_sleep
	queue_add(proc_sleep, &head_sleep, Process, queue, wake);
/*	Process *ptr_elem = proc_sleep;
	link *head = &head_sleep;

	do {                                                                  \
		link *__cur_link=head;                                        \
		Process *__elem = (ptr_elem);                                    \
		link *__elem_link=&((__elem)->queue);                     \
                assert((__elem_link->prev == 0) && (__elem_link->next == 0)); \
		do  __cur_link=__cur_link->next;                              \
	   	while ( (__cur_link != head) &&                               \
		        (((queue_entry(__cur_link,Process,queue))->wake)\
	     	               < ((__elem)->wake)) );                    \
	   	__elem_link->next=__cur_link;                                 \
	   	__elem_link->prev=__cur_link->prev;                           \
	   	__cur_link->prev->next=__elem_link;                           \
	   	__cur_link->prev=__elem_link;                                 \
	} while (0);*/

	ordonnance();
}

/**
 * Passe le processus dans l'état endormi
 * pendant un certain nombre de secondes.
 */
void sleep(uint32_t seconds)
{
	wait_clock(seconds * SCHEDFREQ + current_clock());
}

void idle(void)
{
	sti();

	for (;;) {
		hlt();
		// affiche_etats();
	}
}

static inline bool is_killable(int pid)
{
	// Tous sauf idle
	if (0 < pid || pid < MAX_NB_PROCS)
		return true;

	return false;
}

// Réveil du WAITPID si besoin
static void waitpid_end(int pid)
{
	if (!is_killable(pid))
		return;

	const Process *proc = procs[pid];
	const int ppid = proc->ppid;

	// S'il y a un père
	if (ppid > 0) {
		Process *parent = procs[ppid];

		// Réactivation du parent si besoin
		if (parent->state == WAITPID
			&& (parent->s.waitpid == pid || parent->s.waitpid < 0)) {
			parent->s.waitpid = pid;
			parent->state = ACTIVABLE;

			pqueue_add(parent, &head_act);
		}
	}
}

static inline void del_child(Process **pchild)
{
	Process *child = *pchild;

	if (child != NULL) {
		queue_del(child, children);
		*pchild = NULL;
	}
}

bool is_zombie(int pid)
{
	if (is_killable(pid)) {
		Process *proc = procs[pid];

		if (proc != NULL && proc->state == ZOMBIE)
			return true;
	}

	return false;
}

bool finish_process(int pid)
{
	// Gestion des erreurs
	if (!is_killable(pid) || is_zombie(pid))
		return false;

	enum State state = DYING;
	Process *proc = procs[pid];
	Process *it = NULL;
	Process *die = NULL;

	cqueue_for_each(it, &proc->head_child) {

		del_child(&die);

		// On invalide le père des fils
		it->ppid = -1;

		// On détruit les zombies
		if (it->state == ZOMBIE) {
			die = it;
			die->state = DYING;
			pqueue_add(it, &head_dead);
		}
	}

	del_child(&die);

	if (proc->ppid >= 0) {
		state = ZOMBIE;
	}

	proc->state = state;

	waitpid_end(pid);

	return true;
}

// Termine le processus courant
void fin_processus()
{
	// Si pas de processus à tuer différent de idle, abandon
	if (cur_proc == NULL)
		return;

	finish_process(cur_proc->pid);

	//printf("fin processus %s pid = %i\n", mon_nom(), mon_pid());

	ordonnance();
}

// Cree le processus idle (pas besoin de stack)
// Il faut l'appeler en premier dans kernel_start
bool init_idle()
{
	Process *proc = mem_alloc(sizeof(Process));

	if (proc != NULL) {
		const int32_t pid = 0;

		proc->prio = 0;
		proc->ppid = pid;
		proc->pid = pid;
		strcpy(proc->name, "idle");
		INIT_LIST_HEAD(&proc->head_child);

		procs[pid] = proc;

		// idle est le processus initial
		cur_proc = proc;
		proc->state = CHOSEN;

		nb_procs = 1;
	}

	return (proc != NULL);
}

//void *alloc_page()
//{
//	return mem_alloc(PAGESIZE*2);
//}
// TODO

//void free_page(void *page)
//{
// TODO
//}

extern unsigned pgdir[];

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

	if (nb_procs < MAX_NB_PROCS && name != NULL && is_valid_prio(prio)) {
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
				pid = proc->pid = nb_procs++;
				INIT_LIST_HEAD(&proc->head_child);

				// Copie du nom de processus
				strncpy(proc->name, name, PROC_NAME_SIZE);
				proc->name[PROC_NAME_SIZE-1] = 0;

				// On définit le père et on l'ajoute à la liste de ses fils
				proc->ppid = getpid();

				Process *parent = procs[proc->ppid];

				if (parent != NULL)
					queue_add(proc, &parent->head_child, Process, children, pid);

				// On met l'adresse de terminaison du processus en sommet de pile
				// pour permettre son auto-destruction
				proc->stack[stack_size - 1] = (int)arg;
				proc->stack[stack_size - 2] = (int)fin_processus;
				proc->stack[stack_size - 3] = (int)pt_func;

				// On initialise esp sur le sommet de pile désiré
				proc->regs[ESP] = (int)&proc->stack[stack_size - 3];

				/*
				proc->pdir = (int32_t*)alloc_page();*/

				//proc->pdir = (int32_t*)((int)proc->pdir + (PAGESIZE-(int)proc->pdir%PAGESIZE));
				//printf("proc->pdir = %08X\n", (uint32_t)proc->pdir);

				// proc->ptable = (int32_t*)alloc_page();

				// Initialisation du page directory
				// memset(proc->pdir, 0, PAGESIZE);

				// // Initialisation de la table des pages
				// memset(proc->ptable, 0, PAGESIZE);

				// proc->pdir[0] = (uint32_t)pgdir;
				// proc->pdir[1] = (uint32_t)proc->ptable;

				// // Allocation d'une première page
				// proc->ptable[0] = (uint32_t)alloc_page();

				/*
				// Copie du mapping kernel
				memcpy(proc->pdir, pgdir, PAGESIZE);*/

				// dummy for now
				proc->pdir = (int32_t*)pgdir;
				// proc->ptable


				// Enregistrement dans la table des processus
				procs[pid] = proc;

				// Ajout aux activables
				proc->state = ACTIVABLE;
				pqueue_add(proc, &head_act);

				// Si le processus créé est plus prioritaire,
				// on lui passe la main
				if (prio > cur_proc->prio) {
					ordonnance();
				}
			}
			else
				mem_free_nolength(proc);
		}
	}

	return pid;
}

/**
 * Le processus appelant est terminé normalement et la valeur retval est passée
 * à son père quand il appelle waitpid. La fonction exit ne retourne jamais à
 * l'appelant.
 */
void _exit(int retval)
{
	cur_proc->s.retval = retval;
	fin_processus();
}

/**
 * La primitive kill termine le processus identifié par la valeur pid.
 * Si la valeur de pid est invalide, la valeur de retour est strictement
 * négative. En cas de succès, la valeur de retour est nulle.
 */
int kill(int pid)
{
	int ret = -1;

	if (!is_killable(pid) || is_zombie(pid))
		return ret;

	Process *proc = procs[pid];
	proc->s.retval = 0;
	bool is_other = (proc != cur_proc);

	// Gestion des autres processus
	if (is_other)  {
		enum State state = proc->state;

		switch (state) {
		case ACTIVABLE:
		case ASLEEP:
			// On retire proc des activables / dormants
			queue_del(proc, queue);
			break;

		case ZOMBIE:
			// printf(" ZOMBIE");
			break;

		case WAITPID:
			// printf(" WAITPID");
			break;

		case CHOSEN:
			// printf(" CHOSEN");
			break;

		case DYING:
			// printf(" DYING");
			break;

		default:
			// printf(" ??");
			// Rien à faire
			break;
		}
	}

	if (finish_process(pid)) {

		if (is_other) {

			// On le tue directement si ce n'est pas le processus actuel
			// et qu'il n'est pas devenu un zombie
			if (proc->state == DYING) {
				//pqueue_add(proc, &head_dead); // quid crash observé sur ceci?
				free_process(proc); // Intéret d'une liste de mourants?!!
			}
		}

		// En cas d'autokill
		else {
			ordonnance();
		}

		ret = 0;
	}


	return ret;
}

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
int waitpid(int pid, int *retvalp)
{
	int ret = -1;
	bool wait_child = false;
	bool is_zombie = false;
	Process *child = NULL;

	// Si le pid est valide, il correspond à un pid réel
	if (is_killable(pid)) {

		// On s'assure que pid est l'un des fils du processus actuel
		cqueue_for_each(child, &cur_proc->head_child) {

			if (child->pid == pid) {
				child = procs[pid];

				if (child != NULL) {
					wait_child = true;

					if (child->state == ZOMBIE) {
						is_zombie = true;
					}
				}
				break;
			}
		}
	}

	// Si pid est négatif et que le processus possède au moins un fils,
	// on attend n'importe quel fils
	else if (pid < 0 && !queue_empty(&cur_proc->head_child)) {
		wait_child = true;

		// On cherche si un zombie est déjà présent
		cqueue_for_each(child, &cur_proc->head_child) {
			child = procs[child->pid];

			if (child && child->state == ZOMBIE) {
				is_zombie = true;
			}
		}
	}

	if (wait_child) {

		if (is_zombie) {
			ret = child->pid;
		}
		else {
			// On passe le processus en état d'attente
			cur_proc->s.waitpid = pid;
			cur_proc->state = WAITPID;

			ordonnance();

			// On récupère le pid et le retour du fils débloqué
			ret = cur_proc->s.waitpid;
			child = procs[ret];
		}

		if (retvalp != NULL) {
			*retvalp = child->s.retval;
		}

		// On supprime le fils débloqué des fils
		queue_del(child, children);

		// On le détruit
		// child->state = DYING;
		// pqueue_add(child, &head_dead);
		free_process(child);
	}

	return ret;
}

/**
 * Renvoie la priorité du processus de pid correspondant.
 * Si le pid est invalide, renvoie une valeur négative.
 */
int getprio(int pid)
{
	if (0 <= pid && pid < MAX_NB_PROCS && !is_zombie(pid)) {
		Process *proc = procs[pid];

		if (proc != NULL) {
			return proc->prio;
		}
	}

	return -1;
}

/**
 * Donne la priorité newprio au processus identifié par la valeur de pid.
 * Si la valeur de newprio ou de pid est invalide, la valeur de retour
 * est strictement négative, sinon elle est égale à l'ancienne
 * priorité du processus pid.
 */
int chprio(int pid, int newprio)
{
	if (!is_killable(pid) || is_zombie(pid) || !is_valid_prio(newprio))
		return -1;

	Process *proc = procs[pid];

	if (proc == NULL)
		return -2;

	int prio = proc->prio;
	proc->prio = newprio;

	// Si la priorité d'un processus activable a changé,
	// on le réinsère dans la file des activables
	if (prio != newprio && proc->state == ACTIVABLE) {
		queue_del(proc, queue);
		pqueue_add(proc, &head_act);
	}

	return prio;
}

