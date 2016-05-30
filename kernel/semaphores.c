#include "queue.h"
#include "process.h"
#include "cpu.h"
#include "semaphores.h"
#include <stdbool.h>
#include <string.h>

#define NB_MAX_SEMA 10000

static semaphore tab_sema [2*NB_MAX_SEMA];
static unsigned int nb_sema=0;
static int prochain_id=1;
static int tab_id [2*NB_MAX_SEMA];
static link tab_link [2*NB_MAX_SEMA];

void init_sema()
{
	nb_sema = 0;
	prochain_id = 1;

	memset(&tab_id, 0, sizeof(tab_id));
	memset(&tab_link, 0, sizeof(tab_link));
	memset(&tab_sema, 0, sizeof(tab_sema));
}

static int nvelle_place (unsigned int id) {

	int premiere_place = id % NB_MAX_SEMA;
	int seconde_place = premiere_place + NB_MAX_SEMA;
	return (tab_id[premiere_place] == 0) ? premiere_place 
		: seconde_place;
}

static int get_pos_semaphore (int id) {
	
	int premiere_place = id % NB_MAX_SEMA;
	int seconde_place = premiere_place + NB_MAX_SEMA;

	if (tab_id[premiere_place] == id)
		return premiere_place;

	if (tab_id[seconde_place] == id)
		return seconde_place;
	
	return -1;		
}

static semaphore *get_semaphore (int id) {
	if (id < 0)
		return 0;

	int pos = get_pos_semaphore (id);
	if (pos < 0)
		return 0;

	return &(tab_sema[pos]);
}

int screate (short int count) {
	if (nb_sema >= NB_MAX_SEMA || count < 0)
		return -1;
	nb_sema ++;
	int res = prochain_id;
	int place = nvelle_place(prochain_id);
	tab_id[place] = prochain_id;

	//static list l = LIST_HEAD_INIT(l);
	tab_sema[place].cpt = count;
	tab_sema[place].file = &(tab_link[place]);
	tab_link[place].next=&(tab_link[place]);
	tab_link[place].prev=&(tab_link[place]);
	prochain_id ++;

	return res;

}

int scount (int sem) {

	semaphore *s = get_semaphore(sem);
	return (s == 0) ? -1 : 0xFFFF & s -> cpt;
}

static void debloquer_sema (semaphore *s, uint8_t code) {
	while (true) {

		Process *p = queue_out(s->file, Process, sema_queue);
		if (p  != 0) 
			debloque_sema(p, code);
		else
			break;
	}	
}

int sdelete(int sem) {
	if (sem < 0)
		return -1;

	int pos = get_pos_semaphore(sem);
	if (pos == -1)
		return -1;

	semaphore *s = &(tab_sema[pos]);
	if (s == 0)
		return -1;
	tab_id[pos] = 0;
	debloquer_sema(s, 3);
	nb_sema --;
	return 0;
	
}


int sreset(int sem, int count) {
	semaphore *s = get_semaphore(sem);
	if (s==0 || count <0)
		return -1;

	debloquer_sema(s, 4);
	s->cpt = count;
	
	return 0;
	
}

static int test_wait(int sem) {
	semaphore *s = get_semaphore(sem);
	if (s==0)
		return -1;
	
	if ((int16_t) (s->cpt-1) > s->cpt)
		return -2;
	
	return 0;
}


int try_wait(int sem) {
	int test = test_wait(sem);
	if (test < 0)
		return test;

	semaphore *s = get_semaphore(sem);
	//printf("semaphore numero %i ; cpt = %i\n",sem, s->cpt);
	if (s->cpt <= 0)
		//Cas où on bloquerait le processus si on décrementait le compteur
		return -3;

	s->cpt--;
	return 0;
}


int wait (int sem) {
	//affiche_etats();
	
	/*semaphore *s1 = get_semaphore(1);
	printf("%p ",s1->file);

	s1 = get_semaphore(2);
	printf("%p ",s1->file);

	s1 = get_semaphore(3);
	printf("%p ",s1->file);

	*((int *)0)=1;*/


	int test = test_wait(sem);
	if (test < 0)
		return test;

	semaphore *s = get_semaphore(sem);

	s->cpt--;

	if (s->cpt<0) {
		//printf("proc %i attend sema %i ; cpt = %i\n",getpid() ,sem, s->cpt);

		Process *p = get_cur_proc();
		INIT_LINK(&p->sema_queue);
		p -> sema = s;

		queue_add(p,(s->file),Process,sema_queue,prio);
		
		bloque_sema(s);
		return -get_code_reveil();
	}
	//printf("fin wait\n");
	return 0;
	
}

int signaln (int sem, short int count) {

	semaphore *s = get_semaphore(sem);
	if (s==0)
		return -1;
	//printf("le proc %i debloque le semaphore numero %i ; cpt = %i\n",getpid() ,sem, s->cpt);

	//Si la capacité du compteur est dépassée
	if ( (int16_t) (s->cpt+count) < s->cpt)
		return -2;	
	for (int i = 0; i < count; i++) {
		s -> cpt ++;
		if (s->cpt<=0) {
			//printf("proc %i debloque sema %i ; cpt = %i\n",getpid() ,sem, s->cpt);
			Process *p = queue_out(s->file, Process, sema_queue);
			if (p !=0) {
				//printf("%i ", p->pid);
				debloque_sema(p, 0);
			}
		}

	}

	if (s->cpt<=0)
		ordonnance();
	return 0;

}

int signal (int sem) {
	//printf("Entree signal\n");
	return signaln(sem,1);
	//printf("Sortie signal\n");

}
