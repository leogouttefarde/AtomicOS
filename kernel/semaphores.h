#include "queue.h"
#include <stdint.h>

typedef struct semaphore{
	int16_t cpt;
	link *file;
} semaphore;

int screate (short int count);
int scount (int sem);
int wait (int sem);
int signal (int sem);
int signaln (int sem, short int count);
int try_wait(int sem);
int sdelete(int sem);
int sreset(int sem, int count);

