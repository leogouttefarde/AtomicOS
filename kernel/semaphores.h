#include "queue.h"

typedef struct semaphore{
	int cpt;
	link *file;
} semaphore;


semaphore init_semaphore (int c);
void p(semaphore *s) ;
void v(semaphore *s);

