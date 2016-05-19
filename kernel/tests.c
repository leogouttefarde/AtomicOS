
#include "tests.h"
#include "process.h"
#include "debug.h"


int pexit(void *args)
{
	// printf(" 5");
	_exit((int) args);
	assert(0);
	return 0;
}

int pkill(void *args)
{
	while (1) { // problème si pas de while ni sleep : kill impossible sur un zombie (quid du test fourni?)
		// printf(" X");
		sleep(1);
	}

	return (int)args;
}

void test_kill_exit()
{
	// Test kill / exit fourni
	int rval;
	int r;
	int pid1;
	int val = 45;

	// printf("1");
	pid1 = start("procKill", 4000, 100, (void *) val, pkill);
	assert(pid1 > 0);
	// printf(" 2");
	r = kill(pid1);
	assert(r == 0);
	// printf(" 3");
	r = waitpid(pid1, &rval);
	assert(rval == 0);
	assert(r == pid1);
	// printf(" 4");
	pid1 = start("procExit", 4000, 192, (void *) val, pexit);
	assert(pid1 > 0);
	// printf(" 6");
	r = waitpid(pid1, &rval);
	assert(rval == val);
	assert(r == pid1);
	assert(waitpid(getpid(), &rval) < 0);
	// printf(" 7.\n");
}

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
	start("hw31", 1024, 17, (void*)31, hello_world);
	start("hw20", 1024, 12, (void*)20, hello_world);
	start("hw30", 1024, 17, (void*)30, hello_world);
	start("hw21", 1024, 12, (void*)21, hello_world);
}

