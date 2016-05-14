#include "debugger.h"
#include "cpu.h"
#include "screen.h"
#include <stdio.h>

int fact(int n)
{
	if (n < 2)
		return 1;

	return n * fact(n-1);
}


void kernel_start(void)
{
	int i;
	printf("\f");
	banner();
//	call_debugger();

	i = 10;

	i = fact(i);

	while(1)
	  hlt();

	return;
}
