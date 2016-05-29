#include "debugger.h"
#include "cpu.h"
#include "screen.h"
#include <stddef.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#include "console.h"
#include "messages.h"

void kernel_start(void)
{
	printf("\f");
	banner();
	//	call_debugger();

	init_process();
	init_time();
	init_clavier();
	init_messages();

	start("autotest", 4000, 10, NULL);
	// start("test18", 4000, 10, NULL);
	start("shell", 4000, 10,NULL);

	idle();

	// On ne doit jamais sortir de kernel_start
	while (1) {

		// Cette fonction arrete le processeur
		hlt();
	}

	return;
}
