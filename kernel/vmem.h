
#ifndef __VMEM_H__
#define __VMEM_H__

#include <process.h>
#include <stdint.h>
#include "userspace_apps.h"

void *alloc_page();

int free_page(void *page);

// Gets the physical address of a virtual address
void *get_physaddr(uint32_t *pd, void *virtualaddr);

// Maps a virtual address to a physical address
bool map_page(uint32_t *pd, void *physaddr, void *virtualaddr, uint16_t flags);

bool alloc_pages(Process *proc, struct uapps *app);

#endif
