
#include <stdio.h>
#include <cpu.h>
#include <stddef.h>
#include <string.h>
#include "process.h"
#include "time.h"
#include "mem.h"

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

/* Supprime l'élément de list qui suit l'élément last.
 * S'il est NULL, supprime la tête */
static inline void remove(ListProc *list, Process *last)
{
	Process **proc;

	if (!list || !(proc = &last->u.next))
		return;

	if (last == NULL)
		pop_head(list);

	else {
		Process *next = (*proc)->u.next;
		*proc = next;

		if (next == NULL) {
			list->tail = last;
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

		// Si l'état du processus reste inchangé,
		// on le remet avec les activables
		else if (prev->state == CHOSEN) {
			prev->state = ACTIVABLE;
			INIT_LINK(&prev->u.lnext);
			queue_add(prev, &head_act, Process, u.lnext, prio);
		}

		// On extrait le processus suivant
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

bool is_killable(int pid)
{
	// Tous sauf idle
	if (pid <= 0 || pid >= MAX_NB_PROCS)
		return false;

	return true;
}

// Réveil du WAITPID si besoin
void waitpid_end(int pid)
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

			INIT_LINK(&parent->u.lnext);
			queue_add(parent, &head_act, Process, u.lnext, prio);
		}
	}
}

bool finish_process(int pid)
{
	// Si pas de processus à tuer différent de idle, abandon
	if (!is_killable(pid))
		return false;

	Process *proc = procs[pid];

	// On ignore les zombies
	if (proc == NULL || proc->state == ZOMBIE)
		return false;

	ListProc *children = &proc->children;
	Process *it = children->head;
	Process *last = NULL;
	Process *next;

	while (it != NULL) {
		next = it->u.next;

		// On invalide le père des fils
		it->ppid = -1;

		// On détruit les zombies
		if (it->state == ZOMBIE) {
			remove(children, last);
			add_tail(&list_dead, it);
		}
		else {
			last = it;
		}

		it = next;
	}

	enum State state = DYING;

	if (proc->ppid > 0) {
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

	if (finish_process(pid)) {
		Process *proc = procs[pid];
		proc->s.retval = 0;

		// Gestion des autres processus
		if (proc != cur_proc)  {
			enum State state = proc->state;

			switch (state) {
			case ACTIVABLE:
				// On retire proc des activables
				queue_del(proc, u.lnext);
				break;

			case ASLEEP:
				// TODO : retirer proc des asleeps
				// remplacer implém par dbly linked lists C IG
				break;

			default:
				// Rien à faire
				break;
			}

			add_tail(&list_dead, procs[pid]);
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
	bool success = false;

	if (pid > 0) {

		// TODO : recherche du fils de pid correspondant dans children

		success = true;
	}

	else if (pid < 0) {
		success = true;
	}

	if (success) {

		// On passe le processus en état d'attente
		cur_proc->s.waitpid = pid;
		cur_proc->state = WAITPID;

		ordonnance();

		// On récupère le pid et le retour du fils débloqué
		ret = cur_proc->s.waitpid;

		if (retvalp != NULL) {
			*retvalp = procs[ret]->s.retval;
		}

		// Destruction du fils débloqué

		// TODO : on le supprime des fils

		// add_tail(&list_dead, fils_debloqué);
	}

	return ret;
}

/**
 * Renvoie la priorité du processus de pid correspondant.
 * Si le pid est invalide, renvoie une valeur négative.
 */
int getprio(int pid)
{
	if (0 <= pid && pid < MAX_NB_PROCS) {
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
// int chprio(int pid, int newprio)
// {
/* TODO
http://ensiwiki.ensimag.fr/index.php/Projet_syst%C3%A8me_:_sp%C3%A9cification#chprio_-_Changement_de_priorit.C3.A9_d.27un_processus
*/
// }

