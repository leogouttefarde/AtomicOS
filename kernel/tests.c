
#include "tests.h"
#include "process.h"
#include "debug.h"


int hello_world(void *a)
{
	while(1) {
		sleep(1);
		printf("Hello World ! %d\n\n", (int)a);
	}

	return 0;
}

void test_arg_stack()
{
	// start("hw31", 1024, 17, (void*)31, hello_world);
	// start("hw20", 1024, 12, (void*)20, hello_world);
	// start("hw30", 1024, 17, (void*)30, hello_world);
	// start("hw21", 1024, 12, (void*)21, hello_world);
}

