/*
 * Copyright (C) 2005 -- Simon Nieuviarts
 * Copyright (C) 2012 -- Damien Dejean <dam.dejean@gmail.com>
 *
 * Memory allocator. The three first functions will be satisfied at link time as
 * they are provided by the allocator contained in the shared library.
 */
#ifndef __MEM_H__
#define __MEM_H__

void *mem_alloc(unsigned long length);
void mem_free(void *zone, unsigned long length);
void mem_free_nolength(void *zone);

int phys_init();
void *phys_alloc(unsigned long size);
int phys_free(void *ptr, unsigned long size);
int phys_destroy();

#endif
