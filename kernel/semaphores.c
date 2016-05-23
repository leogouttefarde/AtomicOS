#include "queue.h"
#include "process.h"
#include "cpu.h"
#include "semaphores.h"

typedef struct elt{
	int pid;
	link lien;
	int prio;
} elt;

semaphore init_semaphore (int c) {
	cli();

	static link l=LIST_HEAD_INIT(l);
	semaphore s = {c,&l};

	sti();
	return s;

}

void p(semaphore *s) {
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
    
}

void v(semaphore *s) {
	cli(); //Masquage des it
	s->cpt++;

	if (s->cpt<=0) {
		int pid = (queue_out(s->file, elt, lien))->pid;
		debloque_sema(pid);    
	}
	sti();//Demasquage des it
  
}
