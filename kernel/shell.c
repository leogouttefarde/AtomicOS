#include "console.h"
#include <stdbool.h>
#include <string.h>

static void interpreter (const char *ligne) {
	if (strcmp("echo",ligne) == 0) {
		cons_echo(0);
	}
	else {
		cons_write("commande introuvable\n",21);
	}
}

int shell (void *p) {
	if (p==NULL) {}
	while(true) {
		cons_write(">",1);
		char test [10];
		unsigned long size = cons_read(test,9);
		test[size+1]='\0';
		interpreter(test);
		//cons_write("\n",1);
	}
}
