#include "semaphores.h"
#include <stdbool.h>
#include "cpu.h"
#include "process.h"

#define TAILLE_TAB 8

int tab [TAILLE_TAB];
unsigned int i_lec=0;
unsigned int i_ecr=0;
int rempli, vide, m1, m2;
/*semaphore rempli;
semaphore vide;
semaphore m1,m2;*/

void init () {
	rempli = screate(0);
	vide = screate(TAILLE_TAB);
	m1 = screate(1);
	m2 = screate(1);	
}


int producteur (void *arg) {
	printf("yes");

	if (arg==0) {}
	int i=1;
	while (i<20) {
		wait(vide);

		wait(m1);

		printf("depot de %i\n", i);
		tab[i_ecr]=i;
		i_ecr=(i_ecr+1) % TAILLE_TAB;
		i++;

		signal(m1);

		signal(rempli);
    
	}
	return 0;
}

int consommateur (void *arg) {
		printf("yes");
	
	if (arg==0) {}
  
	while (true) {
		wait(rempli);

		wait(m2);

		int msg=tab[i_lec];
		i_lec = (i_lec+1) % TAILLE_TAB;
		printf("%i recupere\n", msg);
		signal(m2);


		signal(vide);
      
	}
	return 0;
}
