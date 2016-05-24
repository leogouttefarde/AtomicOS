#include "queue.h"
#include "process.h"
#include "cpu.h"
#include "semaphores.h"
#include <stdbool.h>

#define NB_MAX_SEMA 50

typedef struct elt{
	int pid;
	link lien;
	int prio;
} elt;

semaphore tab_sema [NB_MAX_SEMA];
bool tab_occup [NB_MAX_SEMA]; /*tableau représentant les cases occupées dans
				   tab_sema*/

unsigned int nb_sema=0; //Nombre de sémaphores dans tab_sema

static unsigned int recherche_case_vide () {
	/*Pour plus d'efficacité, on pourrait implémenter une liste de cases 
	  vides*/
	unsigned int i = 0;
	while (tab_occup[i])
		i++;
	return i;
}

static semaphore init_semaphore (int c) {

	//Initialisation de la file d'attente associée au sémpahore
	static link l=LIST_HEAD_INIT(l);
	semaphore s = {c,&l};
	return s;
}

int screate (short int count) {
	
	cli();
	if (nb_sema >= NB_MAX_SEMA || count < 0) {
		sti ();
		return -1;
	}

	//Création d'un sémaphore dont le pointeur sera rangé dans tab_sema 
	int res = recherche_case_vide();	
	tab_sema[res] = init_semaphore (count);
	tab_occup[res]=true;	
	
	nb_sema++;

	sti(); //A changer ?
	return res;
}

int scount (int sem) {
	if (!tab_occup[sem]) 
		return -1;
	
	return tab_sema[sem].cpt;
}


int wait (int sem) {
	//PENSER A VERIFIER SUR ENSIwiKI
	//En particulier code d'erreur -3 et -4
	
	//Correspond à l'opération P
	cli();
	
	//Si la valeur du semaphore est invalide
	if (!tab_occup[sem]) {
		sti();
		return -1;
	}
	
	semaphore s = tab_sema[sem];
	//Si la capacité du compteur est dépassée
	if (s.cpt-1 > s.cpt) {
		sti ();
		return -2;
	}

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
	}
	sti();
	return 0;
}

int signal (int sem) {
	if (!tab_occup[sem]) {
		sti();
		return -1;
	}
	
	semaphore s = tab_sema[sem];
	//Si la capacité du compteur est dépassée
	if (s.cpt+1 < s.cpt) {
		sti ();
		return -2;
	}

	if (s.cpt <= 0) {
		int pid = (queue_out(s.file, elt, lien))->pid;
		debloque_sema(pid);    
	}
	sti();//Demasquage des it
	return 0;

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
