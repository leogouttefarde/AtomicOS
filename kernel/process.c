#include <stdio.h>
#include <cpu.h>
#include <stddef.h>
#include <string.h>
#include "process.h"
#include "time.h"
#include "mem.h"
#include "apps.h"
#include "interrupts.h"
#include "console.h"
#include "syscalls.h"
#include "shmem.h"
#include "vmem.h"
#include "semaphores.h"
#include "messages.h"
#include "screen.h"
#include "processor_structs.h"
#include "vesa.h"

// Nombre de processus créés depuis le début
static int32_t nb_procs = -1;

// Processus
static Process* procs[MAX_NB_PROCS];
static Process *cur_proc = NULL;

// Listes de gestion des processus
static link head_act = LIST_HEAD_INIT(head_act);
static link head_sleep = LIST_HEAD_INIT(head_sleep);
static link head_dead = LIST_HEAD_INIT(head_dead);
// static link head_sema = LIST_HEAD_INIT(head_sema);
static link head_io = LIST_HEAD_INIT(head_io);

typedef struct FreePid_ {
	int pid;
	link queue;
} FreePid;

static link head_pid = LIST_HEAD_INIT(head_pid);
static int pid_index = 1;


void ctx_sw(uint32_t *old_ctx, uint32_t *new_ctx, uint32_t **old_cr3, uint32_t *new_cr3);
void start_proc();

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

			case ZOMBIE:
				state = "ZOMBIE";
				break;

			case WAITPID:
				state = "WAITPID";
				break;
				
			case BLOCKEDSEMA:
				state = "BLOCKEDSEMA";
				break;
				
			case WAITMSG:
				state = "WAITMSG";
				break;

			case WAITIO:
				state = "WAITIO";
				break;

			default:
				break;
			}

			printf("    processus numero %d : %s dans l'etat %s\n", proc->pid, proc->name, state);
		}
	}
}

Process *get_cur_proc()
{
	return cur_proc;
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

int getppid()
{
	if (cur_proc == NULL)
		return -1;

	return cur_proc->ppid;
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

// Libération de chaque zone partagée utilisée
void free_process_shmem(void *key,
	__attribute__((__unused__)) void *value, void *arg)
{
	shm_release_proc((const char*)key, (Process*)arg);
}

static inline void free_process(Process *proc)
{
	if (proc == NULL || !proc->pid)
		return;

	int pid = proc->pid;

	FreePid *fpid = mem_alloc(sizeof(FreePid));
	fpid->pid = pid;

	INIT_LINK(&fpid->queue);
	queue_add(fpid, &head_pid, FreePid, queue, pid);

	// Libération des zones partagées utilisées
	hash_for_each(&proc->shmem, (void*)proc, free_process_shmem);
	hash_destroy(&proc->shmem);

	if (proc->pdir != NULL) {
		void *page = NULL, *vpage = NULL;

		// Libération de la pile utilisateur
		for (uint32_t i = 0; i < proc->spages; i++) {
			vpage = get_ustack_vpage(i, proc->spages);
			assert(vpage);
			page = get_physaddr(proc->pdir, vpage);
			assert(page);
			free_page(page);
		}

		// Libération de la pile kernel
		phys_free(proc->kstack, KERNELSSIZE);

		// Libération du code utilisateur
		for (uint32_t i = 0; i < proc->cpages; i++) {
			vpage = get_ucode_vpage(i);
			assert(vpage);
			page = get_physaddr(proc->pdir, vpage);
			assert(page);
			free_page(page);
		}

		// // Libération du page directory
		if (proc->pdir != NULL) {
			uint32_t entry;

			// Peu efficace ?
			for (uint32_t i = 0; i < PD_SIZE; i++) {
				entry = proc->pdir[i];

				if (entry > PAGESIZE) {
					free_page((void*)(entry & ~0xFFF));
				}
			}
		}

		free_page(proc->pdir);
	}

	procs[pid] = NULL;
	mem_free_nolength(proc);

	nb_procs--;
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
		add_proc_activable(proc);
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


//! Code à factoriser !
void bloque_io() {
	Process *proc_io = cur_proc;
	proc_io -> state = WAITIO;
	queue_add(proc_io, &head_io, Process, queue, prio);
	ordonnance();
}

void debloque_io() {
	Process *p = NULL;

	/*queue_for_each(p, &head_io, Process, queue) {

		if (p->pid == pid)
			break;
			}*/
	p = queue_out(&head_io, Process, queue);
	Process *proc = p;
	
	if (proc != NULL) {
		p = NULL;
		proc->state = ACTIVABLE;
		pqueue_add(proc, &head_act);
	}
}

unsigned int get_code_reveil () {
	return cur_proc->code_reveil;
}

void bloque_sema () {
	Process *proc_sema = cur_proc;
	
	proc_sema->state = BLOCKEDSEMA;
	// queue_add(proc_sema, &head_sema, Process, queue, prio);
	ordonnance();
}

void debloque_sema(Process *p, uint8_t code) {
	/*Process *p = NULL;

	queue_for_each(p, &head_sema, Process, queue) {

		if (p->pid == pid)
			break;
	}

	Process *proc = p;
	
	if (proc != NULL) {
		p = NULL;
		queue_del(proc, queue);
		proc->state = ACTIVABLE;
		proc->code_reveil =  code;
		pqueue_add(proc, &head_act);
		}*/

	// queue_del(p, queue);
	p->code_reveil = code;
	add_proc_activable(p);
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

	proc_sleep->state = ASLEEP;
	proc_sleep->wake = clock;

	// Insertion triée du proc_sleep
	queue_add(proc_sleep, &head_sleep, Process, queue, wake);

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

/**
 * Passe le processus dans l'état endormi
 * pendant un certain nombre de tics d'horloge (clocks * 0.02 secondes)
 */
void sleep_clock(uint32_t clocks)
{
	wait_clock(clocks + current_clock());
}

void idle(void)
{
	// sti();

	for (;;) {
		sti();
		hlt();
		cli();
		//affiche_etats();
	}
}

static inline bool is_killable(int pid)
{
	// Tous sauf idle
	if (0 < pid && pid < MAX_NB_PROCS) {
		return true;
	}

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
		free_process(child);
		*pchild = NULL;
	}
}

static inline bool is_zombie(int pid)
{
	if (is_killable(pid)) {
		Process *proc = procs[pid];

		if (proc != NULL && proc->state == ZOMBIE)
			return true;
	}

	return false;
}

static bool finish_process(int pid)
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

			// free manuel dans del_child
			// pqueue_add(it, &head_dead);
		}
	}

	del_child(&die);

	if (proc->state == WAITMSG) {
		queue_del(proc, msg_queue);
		(*proc->msg_counter)--;
	}

	if (proc->state == BLOCKEDSEMA) {
		// queue_del(proc, queue);
		queue_del(proc, sema_queue);
		proc -> sema -> cpt++;
	}

	if (proc->ppid >= 0) {
		state = ZOMBIE;
	}

	proc->state = state;

	waitpid_end(pid);

	return true;
}

void traitant_IT_49();
void exception_IT();
void exception_IT_pop();
void page_fault_IT();

static inline void autokill()
{
	kill(getpid());
}

void page_fault_handler(int address, int error_code)
{
	error_code = error_code;

	printf("[0x%X] : Page fault\n", address);
	autokill();
}

void exception_handler()
{
	printf("ERROR : An exception occured\n");
	autokill();
}

void exception_handler_pop(int error_code)
{
	error_code = error_code;

	printf("ERROR : An exception occured\n");
	// printf("Error code : 0x%X\n", code);
	autokill();
}

// A appeler en premier dans kernel_start
bool init_process()
{
	Process *proc = mem_alloc(sizeof(Process));

	// Cree le processus idle (pas besoin de stack)
	if (proc != NULL) {
		const int32_t pid = 0;

		proc->prio = 0;
		proc->ppid = pid;
		proc->pid = pid;
		strcpy(proc->name, "idle");
		INIT_LIST_HEAD(&proc->head_child);

		memset(&procs, 0, sizeof(procs));
		procs[pid] = proc;

		// idle est le processus initial
		cur_proc = proc;
		proc->state = CHOSEN;

		nb_procs = 1;
	}

	shm_init();
	phys_init();
	init_apps();

	// Syscall handler
	init_traitant_IT_user(49, (int)traitant_IT_49);

	// Error handlers
	init_traitant_IT(0, (int)exception_IT);

	for (uint8_t i = 3; i <= 7; i++)
		init_traitant_IT(i, (int)exception_IT);

	for (uint8_t i = 10; i < 14; i++)
		init_traitant_IT(i, (int)exception_IT_pop);

	init_traitant_IT(16, (int)exception_IT);
	init_traitant_IT(17, (int)exception_IT_pop);
	init_traitant_IT(19, (int)exception_IT);
	init_traitant_IT(20, (int)exception_IT);
	init_traitant_IT(30, (int)exception_IT_pop);

	// Page fault handler
	init_traitant_IT(14, (int)page_fault_IT);

	return (proc != NULL);
}

int get_new_pid()
{
	int pid;

	if (!queue_empty(&head_pid)) {
		FreePid *fpid = queue_bottom(&head_pid, FreePid, queue);
		pid = fpid->pid;

		queue_del(fpid, queue);
		mem_free_nolength(fpid);
	}
	else if (pid_index < MAX_NB_PROCS) {
		pid = pid_index++;
	}
	else {
		panic("FATAL ERROR");
	}

	nb_procs++;

	return pid;
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
int start(const char *name, unsigned long ssize, int prio, void *arg)
{
	int32_t pid = -1;


	if (nb_procs < MAX_NB_PROCS && name != NULL
		&& is_valid_prio(prio) && ssize < PHYS_MEMORY) {

		Process *proc = mem_alloc(sizeof(Process));
		memset(proc, 0, sizeof(Process));

		if (proc == NULL) {
			return pid;
		}

		// Ajout d'une case pour le passage d'argument
		uint32_t stack_size = ssize/4 + 1;

		// Si le nombre n'est pas multiple
		// de sizeof(int), on ajoute une case
		if (ssize % sizeof(int))
			stack_size++;

		proc->ssize = stack_size * sizeof(int);

		// Copie du nom de processus
		strncpy(proc->name, name, PROC_NAME_SIZE);
		proc->name[PROC_NAME_SIZE-1] = 0;

		hash_init_string(&proc->shmem);

		if (alloc_pages(proc)) {

			proc->shm_idx = 0;

			proc->prio = prio;
			pid = proc->pid = get_new_pid();
			INIT_LIST_HEAD(&proc->head_child);

			// On définit le père et on l'ajoute à la liste de ses fils
			proc->ppid = getpid();

			Process *parent = procs[proc->ppid];

			if (parent != NULL)
				queue_add(proc, &parent->head_child,
					Process, children, pid);

			// On met l'adresse de terminaison du processus en sommet de pile
			// pour permettre son auto-destruction
			proc->stack[0x400 - 1] = (uint32_t)arg;
			proc->stack[0x400 - 2] = (uint32_t)start_proc;

			// On initialise esp sur le sommet de pile désiré
			proc->regs[ESP] = USERSTACK - 8;

			// On initialise le sommet de pile kernel
			proc->regs[ESP0] = (uint32_t)&proc->kstack[2047];


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
		else {
			free_process(proc);
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
	// Si pas de processus à tuer différent de idle, abandon
	if (cur_proc == NULL)
		panic("FATAL ERROR\n");

	cur_proc->s.retval = retval;

	finish_process(cur_proc->pid);
	ordonnance();

	panic("FATAL ERROR");
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
	if (proc == NULL) //Si le processus n'existe pas
		return ret;

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
		case BLOCKEDSEMA:
			// queue_del(proc, queue);
			// queue_del(proc, sema_queue);
			// proc -> sema -> cpt++;
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
	Process *child = NULL, *it = NULL;

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
		cqueue_for_each(it, &cur_proc->head_child) {
			it = procs[it->pid];

			if (it && it->state == ZOMBIE) {
				is_zombie = true;
				child = it;
				break;
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

	// Si la priorité d'un processus activable a changé
	if (prio != newprio) {

		// S'il s'agit du processus courant,
		// on force un nouvel ordonnancement
		if (proc == cur_proc) {
			ordonnance();
		}

		// Pour les activables, on réinsère simplement
		// dans la file des activables
		else if (proc->state == ACTIVABLE) {
			queue_del(proc, queue);
			pqueue_add(proc, &head_act);
		}

		// WAITMSG
		else if (proc->state == WAITMSG) {
			queue_del(proc, msg_queue);
			queue_add(proc, proc->blocked_queue, Process, msg_queue, prio);
		}

		//Cas des sémaphores
		else if (proc->state == BLOCKEDSEMA) {
			// queue_del(proc, queue);
			// queue_add(proc, &head_sema, Process, queue, prio);
			queue_del(proc, sema_queue);
			queue_add(proc, proc -> sema -> file, Process, sema_queue, prio);		
		}
	}

	return prio;
}

void sys_info()
{
	printf("TODO : SYS_INFO\n");
}

int syscall(int num, int arg0, int arg1, int arg2, int arg3, int arg4)
{
	// TODO : vérifier les valeurs (notamment pointeurs)
	// pour plus de sécurité
	int ret = -1;

	arg2 = arg2;
	arg3 = arg3;
	arg4 = arg4;

	switch (num) {
	case START:
		if (IS_USER(arg0))
		ret = start((const char*)arg0, arg1, arg2, (void*)arg3);
		break;

	case GETPID:
		ret = getpid();
		break;

	case GETPRIO:
		ret = getprio(arg0);
		break;

	case CHPRIO:
		ret = chprio(arg0, arg1);
		break;

	case KILL:
		ret = kill(arg0);
		break;

	case WAITPID:
		if (IS_USER(arg1))
		ret = waitpid(arg0, (int*)arg1);
		break;

	case EXIT:
		_exit(arg0);
		break;

	case CONS_WRITE:
		if (IS_USER(arg0))
		ret = cons_write((const char*)arg0, arg1);
		break;

	case CONS_READ:
		if (IS_USER(arg0))
		ret = cons_read((char*)arg0, (unsigned long)arg1); 
		break;

	case CONS_ECHO:
		cons_echo(arg0);
		break;

	case SCOUNT:
		ret = scount(arg0);
		break;
		
	case SCREATE:
		ret = screate(arg0);
		break;

	case SDELETE:
		ret = sdelete(arg0);
		break;

	case SIGNAL:
		ret = signal(arg0);
		break;

	case SIGNALN:
		ret = signaln(arg0,arg1);
		break;

	case SRESET:
		ret = sreset(arg0, arg1);
		break;
		
	case TRY_WAIT:
		ret = try_wait(arg0);
		break;
		
	case WAIT:
		ret = wait(arg0);
		break;

	case PCOUNT:
		if (IS_USER(arg1))
		ret = pcount(arg0, (int*)arg1);
		break;

	case PCREATE:
		ret = pcreate(arg0);
		break;

	case PDELETE:
		ret = pdelete(arg0);
		break;

	case PRECEIVE:
		if (IS_USER(arg1))
		ret = preceive(arg0, (int*)arg1);
		break;

	case PRESET:
		ret = preset(arg0);
		break;

	case PSEND:
		ret = psend(arg0, arg1);
		break;

	case CLOCK_SETTINGS:
		if (IS_USER2(arg0, arg1))
		clock_settings((unsigned long*)arg0, (unsigned long*)arg1);
		break;

	case CURRENT_CLOCK:
		ret = current_clock();
		break;

	case WAIT_CLOCK:
		wait_clock(arg0);
		break;

	case SYS_INFO:
		sys_info();
		break;

	case SHM_CREATE:
		if (IS_USER(arg0))
		ret = (int)shm_create((const char*)arg0);
		break;

	case SHM_ACQUIRE:
		if (IS_USER(arg0))
		ret = (int)shm_acquire((const char*)arg0);
		break;

	case SHM_RELEASE:
		if (IS_USER(arg0))
		shm_release((const char*)arg0);
		break;

	case AFFICHE_ETATS:
		affiche_etats();
		break;

	case CONS_SET_FG_COLOR:
		set_fg_color(arg0);
		break;

	case CONS_SET_BG_COLOR:
		set_bg_color(arg0);
		break;

	case CONS_RESET_COLOR:
		reset_color();
		break;

	case REBOOT:
		reboot();
		break;

	case SLEEP:
		sleep(arg0);
		break;

	case GETPPID:
		ret = getppid();
		break;

	case PRINT_BANNER:
		banner();
		break;

	case SET_VIDEO_MODE:
		ret = initGraphics(1366, 768, 32);
		break;

	case GET_VIDEO_MODES:
		list_modes(arg0, arg1);
		break;

	case SET_CURSOR:
		place_curseur(arg0, arg1);
		break;

	case SET_CARACTER:
		ecrit_car(arg0, arg1, arg2);
		break;

	case GETWIDTH:
		ret = get_Width();
		break;

	case GETHEIGHT:
		ret = get_Height();
		break;

	case RESET_INPUTGAME:
		reset_InputGame();
		break;

	case TEST_INPUTGAME:
		ret = test_InputGame();
		break;

	case GET_INPUTGAME:
		ret = get_InputGame();
		break;

	case WAIT_KEYBOARD:
		wait_for_keyboard();
		break;

	default:
		printf("Unknown syscall : %d\n", num);
		break;
	}

	return ret;
}

//Ajout d'un processus dans la liste des processus activable
void add_proc_activable(Process *proc)
{
	if (proc != NULL) {
		proc->state = ACTIVABLE;
		pqueue_add(proc, &head_act);
	}
}

int pid_shell = 0;

void launch_new_shell()
{
	pid_shell = start("shell", 4000, 10, NULL);
}

void kill_recursive(int pid)
{
	Process *proc = procs[pid], *it = NULL;

	queue_for_each (it, &proc->head_child, Process, children) {
		//printf("kill %s\n", it->name);
		kill_recursive(it->pid);
	}

	//printf("kill = %d\n", kill(pid));
}

void abort_shell_wait()
{
	bool stop = false;
	Process *proc = cur_proc, *child = NULL;

	if (!proc->pid && !queue_empty(&head_sleep)) {
		Process *wake;

		do {
			wake = queue_out(&head_sleep, Process, queue);
			add_proc_activable(wake);
		} while (!queue_empty(&head_sleep));
	}
	else {
		while (proc && proc->ppid) {
		//printf("child %s\n", proc->name);
			child = proc;
			proc = procs[proc->ppid];
		//printf("proc %s\n", proc->name);

			if (proc && proc->pid == pid_shell) {
				stop = true;
				break;
			}
		}

		if (stop) {
		//printf("stop %s\n", child->name);

			kill_recursive(child->pid);
		}
		else {
			// if (child)
		//printf("child %s\n", child->name);
		//printf("NULL ? %X\n", (int)proc);
		//printf("ppid %X\n", (int)proc->ppid);
		//printf("name %s\n", proc->name);
		}
	}
}



