#include "debugger.h"
#include "cpu.h"
#include "screen.h"
#include <stddef.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#include "console.h"
#include "tests.h"
#include "test_prod_conso.h"

void kernel_start(void)
{
	printf("\f");
	banner();
	//	call_debugger();

	init_process();
	init_time();
	init_clavier();

	// test_kill_exit();
	// test_arg_stack();
	// init();
	//start("prod", 50000, 10, (void *)50, producteur);
	//start ("conso", 50000, 10, (void *)50, consommateur);

	//start("shell", 1024, 10,NULL);
	start("montest", 11*1024, 10, NULL);
	//start("test0",4000,10,NULL);
	//start("test16", 4000, 128, NULL);
	//start("autotest", 4000, 10, NULL);
	idle();

	// On ne doit jamais sortir de kernel_start
	while (1) {

		// Cette fonction arrete le processeur
		hlt();
	}

	return;
}
