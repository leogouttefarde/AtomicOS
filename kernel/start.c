#include "debugger.h"
#include "cpu.h"
#include "screen.h"
#include <stddef.h>
#include <stdio.h>
#include <process.h>
#include <time.h>

void kernel_start(void)
{
	printf("\f");
	banner();
//	call_debugger();

	init_idle();
	init_temps();

	// start("nom", 10, 2, (void*)777, test45);

	idle();

	// On ne doit jamais sortir de kernel_start
	while (1) {

		// Cette fonction arrete le processeur
		hlt();
	}

	return;
}
