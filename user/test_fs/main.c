
#include <atomic.h>
#include <string.h>
#include <stdio.h>

int main()
{
	int ret;
	char *data0 = "Atomic'OS ";
	char *data1 = "Filesystem ";
	char *data2 = "test\n";

	printf("atomicOpen\n");
	File *file = atomicOpen("testFile.bin");

	if (file) {
		printf("atomicWrite\n");
		atomicWrite(file, data0, strlen(data0));
		atomicWrite(file, data1, strlen(data1));
		atomicWrite(file, data2, strlen(data2));

		printf("atomicClose\n");
		atomicClose(file);
	}

	char buf[32];
	memset(buf, 0, sizeof(buf));

	file = atomicOpen("testFile.bin");

	if (file) {
		ret = atomicRead(file, buf, 255);
		printf("atomicRead = %d\n", ret);
		atomicClose(file);
	}

	if (strcmp(buf, "Atomic'OS Filesystem test\n")) {
		printf("FS checks failed, read this :\n%s", buf);
	}
	else {
		printf("FS checks successfull\n");
	}

	return 0;
}


