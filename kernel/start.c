#include "debugger.h"
#include "cpu.h"
#include "screen.h"
#include <stddef.h>
#include <stdio.h>
#include <process.h>
#include <time.h>

int hello_world(void *a)
{
	while(1) {
		dors(1);
		printf("Hello World ! %d\n\n", (int)a);
	}

	return 0;
}

void kernel_start(void)
{
	printf("\f");
	banner();
//	call_debugger();

	init_idle();
	init_temps();

	start("hw31", 512, 7, (void*)31, hello_world);
	start("hw20", 512, 2, (void*)20, hello_world);
	start("hw30", 512, 7, (void*)30, hello_world);
	start("hw21", 512, 2, (void*)21, hello_world);

	idle();

	// On ne doit jamais sortir de kernel_start
	while (1) {

		// Cette fonction arrete le processeur
		hlt();
	}

	return;
}
