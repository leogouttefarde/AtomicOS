#include "debugger.h"
#include "cpu.h"
#include "screen.h"
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
	idle();

	// On ne doit jamais sortir de kernel_start
	while (1) {

		// Cette fonction arrete le processeur
		hlt();
	}

	return;
}
