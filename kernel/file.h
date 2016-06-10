
#ifndef ___FILE_H___
#define ___FILE_H___

#include <stdint.h>
#include <stdbool.h>

typedef struct File_ File;

void init_fs();

File *atomicOpen(const char *path);

uint32_t atomicRead(File *file, void *data, uint32_t size);

uint32_t atomicWrite(File *file, void *data, uint32_t size);

void atomicClose(File *file);

bool atomicEOF(File *file);

void *atomicData(File *file, uint32_t *size);

void atomicList();

bool atomicExists(char *path);

uint32_t atomicDelete(char *path);

uint32_t atomicCopy(char *path1, char *path2);

uint32_t atomicRename(char *path1, char *path2);

#endif
