#include "queue.h"
#include "process.h"
#include "cpu.h"
#include "semaphores.h"
#include <stdbool.h>

#define NB_MAX_SEMA 500

typedef struct elt{
	//Element a inserer dans la liste de processus associée à un sémaphore
	int pid;
	link lien;
	int prio;
} elt;

semaphore tab_sema [NB_MAX_SEMA];
bool tab_occup [NB_MAX_SEMA]; /*tableau représentant les cases occupées dans
				   tab_sema*/

unsigned int nb_sema=0; //Nombre de sémaphores dans tab_sema

//sti et cli necessaires ?

static semaphore init_semaphore (int c) {

	//Initialisation de la file d'attente associée au sémpahore
	static link l=LIST_HEAD_INIT(l);
	semaphore s = {c,&l};
	return s;
}

int screate (short int count) {

	//cli();
	if (nb_sema >= NB_MAX_SEMA || count < 0) {
		sti ();
		return -1;
	}

	//Création d'un sémaphore dont le pointeur sera rangé dans tab_sema 
	int res = nb_sema+1;	
	tab_sema[res] = init_semaphore (count);
	tab_occup[res] = true;
	nb_sema++;

	//sti(); 
	return res;
}

int scount (int sem) {
	if (!tab_occup[sem]) 
		return -1;
	
	return tab_sema[sem].cpt;
}

static void debloquer_sema (semaphore s, uint8_t code) {
	while (true) {
		int pid = (queue_out(s.file, elt, lien))->pid;
		if (pid  != 0) 
			debloque_sema(pid, code);
		else
			break;
	}	
}

int sdelete(int sem) {
	if (!tab_occup[sem]) 
		return -1;

	semaphore s = tab_sema[sem];
	debloquer_sema(s, 3);
	tab_occup[sem] = false;
	return 0;
}


int sreset(int sem, int count) {
	if (!tab_occup[sem] || count < 0) 
		return -1;

	semaphore s = tab_sema[sem];
	debloquer_sema(s, 4);
	s.cpt = count;
	
	return 0;
}

static int test_wait(int sem) {
	//cli();
	
	//Si la valeur du semaphore est invalide
	if (!tab_occup[sem]) {
		//sti();
		return -1;
	}
	
	semaphore s = tab_sema[sem];
	//Si la capacité du compteur est dépassée
	if (s.cpt-1 > s.cpt) {
		//sti ();
		return -2;
	}
	return 0;
}


int try_wait(int sem) {
	int test = test_wait(sem);
	if (test < 0)
		return test;

	semaphore s = tab_sema[sem];
	if (s.cpt <= 0)
		//Cas où on bloquerait le processus si on décrementait le compteur
		return -3;

	s.cpt--;
	return 0;	
}


int wait (int sem) {
	//cli();
	
	int test = test_wait(sem);
	if (test < 0)
		return test;

	semaphore s = tab_sema[sem];
	s.cpt--;

	if (s.cpt<0) {
		/*Si le cpt est < 0, on ajoute le processus 
		  courant à la file d'attente*/
		elt element;
		element.pid=getpid();
		element.lien.prev=0;
		element.lien.next=0;
		element.prio=1;

		queue_add(&element,(s.file),elt,lien,prio);
		bloque_sema();
		return -get_code_reveil();
	}
	
	//sti();
	return 0;
}

int signaln (int sem, short int count) {
	//cli();

	if (!tab_occup[sem]) {
		//sti();
		return -1;
	}
	
	semaphore s = tab_sema[sem];
	//Si la capacité du compteur est dépassée
	if (s.cpt+count < s.cpt) {
		//sti ();
		return -2;
	}

	if (s.cpt <= 0) {
		for (int i=0; i < count; i++) {
			int pid = (queue_out(s.file, elt, lien))->pid;
			debloque_sema(pid, 0);
		}
	}
	//sti();//Demasquage des it
	return 0;
}

int signal (int sem) {
	return signaln(sem,1);
}


/*void p(semaphore *s) {
	cli(); //Masquage des it
	s->cpt--;

	if (s->cpt<0) {
		elt element;
		element.pid=getpid();
		element.lien.prev=0;
		element.lien.next=0;
		element.prio=1;

		queue_add(&element,(s->file),elt,lien,prio);
		bloque_sema();
	}
	sti();//Demasquage des it
    
	}*/

/*void v(semaphore *s) {
	cli(); //Masquage des it
	s->cpt++;

	if (s->cpt<=0) {
		int pid = (queue_out(s->file, elt, lien))->pid;
		debloque_sema(pid);    
	}
	sti();//Demasquage des it
	}*/
