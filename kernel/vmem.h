
#ifndef __VMEM_H__
#define __VMEM_H__

#include <process.h>
#include <stdint.h>
#include "userspace_apps.h"

#define PAGESIZE  0x1000
#define KERNELSSIZE 0x2000
#define USERCODE  0x40000000
#define USERSTACK 0x80000000

#define get_ustack_vpage(i, nb)	((void*)(USERSTACK - (nb - i) * PAGESIZE))
#define get_ucode_vpage(i)	((void*)(USERCODE + i * PAGESIZE))

void *alloc_page();

int free_page(void *page);

// Gets the physical address of a virtual address
void *get_physaddr(uint32_t *pd, void *virtualaddr);

// Maps a virtual address to a physical address
bool map_page(uint32_t *pd, void *physaddr, void *virtualaddr, uint16_t flags);

bool alloc_pages(Process *proc);

#endif
