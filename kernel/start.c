#include "debugger.h"
#include "cpu.h"
#include "screen.h"
#include <stddef.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#include "console.h"
#include "shell.h"
#include "tests.h"

void kernel_start(void)
{
	printf("\f");
	banner();
//	call_debugger();

	init_idle();

	init_time();
	//init_clavier();

	// test_kill_exit();
	// test_arg_stack();

	// lancer_console();
	start("shell", 1024, 10, (void *)50, shell);
	idle();

	// On ne doit jamais sortir de kernel_start
	while (1) {

		// Cette fonction arrete le processeur
		hlt();
	}

	return;
}
