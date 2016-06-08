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
#include "file.h"

void kernel_start(void)
{
	banner();

	init_process();
	init_time();
	init_clavier();
	init_messages();
	init_sema();
	init_mouse();
	init_fs();

	launch_new_shell();

	idle();

	// On ne doit jamais sortir de kernel_start
	while (1) {

		// Cette fonction arrete le processeur
		hlt();
	}

	return;
}
