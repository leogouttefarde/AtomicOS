#include "debugger.h"
#include "cpu.h"
#include "screen.h"
#include <stddef.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#include "console.h"
#include "messages.h"
#include "mouse.h"
#include "vesa.h"

void kernel_start(void)
{
	banner();
	//	call_debugger();

	init_process();
	init_time();
	init_clavier();
	init_messages();
	init_sema();
	init_mouse();

	// initGraphics(1366, 768, 32);

	// while(1)
	// 	hlt();

	// start("autotest", 4000, 10, NULL);
	// start("test16", 4000, 128, NULL);
	// start("test17", 4000, 128, NULL);
	launch_new_shell();

	idle();

	// On ne doit jamais sortir de kernel_start
	while (1) {

		// Cette fonction arrete le processeur
		hlt();
	}

	return;
}
