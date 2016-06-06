
#include "file.h"
#include "hash.h"
#include "mem.h"
#include <stdio.h>
#include <string.h>

hash_t files;

typedef struct File_ {
	char *name;
	uint32_t pos;
	uint32_t size;
	uint32_t buf_size;
	void *data;
} File;


void init_fs()
{
	hash_init_string(&files);
}

File *atomicOpen(const char *path)
{
	const int LEN = strlen(path);

	if (LEN <= 0)
		return NULL;

	File *file = hash_get(&files, (void*)path, NULL);

	if (file == NULL) {
		char *name = mem_alloc(LEN + 1);

		if (name == NULL) {
			return NULL;
		}

		strcpy(name, path);

		file = (File*)mem_alloc(sizeof(File));

		if (file == NULL) {
			mem_free_nolength(name);
			return NULL;
		}

		memset(file, 0,sizeof(File));
		file->name = name;

		hash_set(&files, (void*)name, (void*)file);
	}
	else {
		file->pos = 0;
	}

	return file;
}

uint32_t atomicRead(File *file, void *data, uint32_t size)
{
	if (!file || !file->data || !data || !size) {
		return 0;
	}

	const uint32_t LEFT = file->size - file->pos;
	uint32_t len = size;

	if (len > LEFT) {
		len = LEFT;
	}

	memcpy(data, (void*)((uint32_t)file->data + file->pos), len);

	return len;
}

uint32_t atomicWrite(File *file, void *data, uint32_t size)
{
	if (file == NULL)
		return 0;

	const uint32_t NEW_POS = file->pos + size;
	const uint32_t LEFT_SPACE = file->buf_size - file->pos;

	if (LEFT_SPACE < size) {

		void *new_buf = mem_alloc(NEW_POS);

		if (new_buf == NULL) {
			return 0;
		}

		if (file->data != NULL) {
			memcpy(new_buf, file->data, file->size);
			mem_free_nolength(file->data);
		}

		file->data = new_buf;
		file->buf_size = NEW_POS;
		file->size = NEW_POS;
	}

	else if (file->data == NULL) {
		return 0;
	}

	memcpy((void*)((uint32_t)file->data + file->pos), data, size);
	file->pos = NEW_POS;

	return size;
}

// Nothing to do
void atomicClose(File *file)
{
	(void)file;
}

// Nothing to do
bool atomicEOF(File *file)
{
	if (file == NULL) {
		return true;
	}

	return (file->pos >= file->size) ? true : false;
}
