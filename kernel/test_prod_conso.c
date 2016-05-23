#include "semaphores.h"
#include <stdbool.h>
#include "cpu.h"
#include "process.h"

#define TAILLE_TAB 8

int tab [TAILLE_TAB];
unsigned int i_lec=0;
unsigned int i_ecr=0;
semaphore rempli;
semaphore vide;
semaphore m1,m2;

void init () {
	rempli=init_semaphore(0);
	vide=init_semaphore(TAILLE_TAB);
	m1=init_semaphore(1);
	m2=init_semaphore(1);
}


int producteur (void *arg) {
	if (arg==0) {}
	int i=1;
	while (i<20) {
		p(&vide);

		p(&m1);
		printf("depot de %i\n", i);
		tab[i_ecr]=i;
		i_ecr=(i_ecr+1) % TAILLE_TAB;
		i++;
		v(&m1);

		v(&rempli);
    
	}
	return 0;
}

int consommateur (void *arg) {
	if (arg==0) {}
  
	while (true) {
		p(&rempli);

		p(&m2);
		int msg=tab[i_lec];
		i_lec = (i_lec+1) % TAILLE_TAB;
		printf("%i recupere\n", msg);
		v(&m2);

		v(&vide);
      
	}
	return 0;
}
