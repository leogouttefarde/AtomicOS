
#include "vmem.h"
#include <stddef.h>
#include <string.h>
#include <mem.h>

#define PAGESIZE 0x1000

extern unsigned pgdir[];

void *alloc_page()
{
	return phys_alloc(PAGESIZE);
}

int free_page(void *page)
{
	return phys_free(page, PAGESIZE);
}

// Gets the physical address of a virtual address
void *get_physaddr(uint32_t *pd, void *virtualaddr)
{
	uint32_t pdindex = (uint32_t)virtualaddr >> 22;
	uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

	// uint32_t *pd = cur_proc->pdir;
	// printf("phys pd[%X] = %X\n", pdindex, pd[pdindex]);

	// Check if the PD entry is present
	if (pd[pdindex]) {
		uint32_t *pt = (uint32_t*)(pd[pdindex] & ~0xFFF);

	// printf("phys pt[%X] = %X\n", ptindex, pt[ptindex]);
		// Check if the PT entry is present
		if (pt[ptindex]) {
			return (void*)((pt[ptindex] & ~0xFFF) + ((uint32_t)virtualaddr & 0xFFF));
		}
	}
	// printf("get_physaddr fail\n");

	return NULL;
}

// Maps a virtual address to a physical address
bool map_page(uint32_t *pd, void *physaddr, void *virtualaddr, uint16_t flags)
{
	// Make sure that both addresses are page-aligned.
	if ((uint32_t)physaddr % PAGESIZE || (uint32_t)virtualaddr % PAGESIZE)
		return false;

	uint32_t pdindex = (uint32_t)virtualaddr >> 22;
	uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

	// uint32_t *pd = cur_proc->pdir;
	// printf("m pdindex %X\n", pdindex);

	// If the PD entry is not present, create it
	if (!pd[pdindex]) {
	// printf("map %X\n", pdindex);

		// TOCHECK
		// Copy the User\Supervisor bit
		uint16_t pdir_flags = (flags & P_USERSUP) | P_RW;
		void *page = alloc_page();
		// printf("new pt -> %X\n", (uint32_t)page);

		assert(page);
		memset(page, 0, PAGESIZE);
		pd[pdindex] = ((uint32_t)page) | (pdir_flags & 0xFFF) | P_PRESENT;
	// printf("pd[%X] = %X\n", pdindex, pd[pdindex]);
	}

	uint32_t *pt = (uint32_t*)(pd[pdindex] & ~0xFFF);
	// printf("pt[%X] = %X\n", ptindex, pt[ptindex]);

	// If the PT entry is not present, map it
	if (!pt[ptindex]) {
		pt[ptindex] = ((uint32_t)physaddr) | (flags & 0xFFF) | P_PRESENT;
		// printf("map : v %X <- p %X\n", (uint32_t)virtualaddr, (uint32_t)physaddr);

		// Flush the entry in the TLB
		tlb_flush();

		return true;
	}

	return false;
}

bool alloc_pages(Process *proc, struct uapps *app)
{
	bool success = true;
	void *page = NULL;

	if (proc == NULL || app == NULL)
		return false;

	// printf("starting %s\n", app->name);

	proc->pdir = (uint32_t*)alloc_page();
	// printf("proc->pdir -> %X\n", (uint32_t)proc->pdir);

	// Initialisation du page directory
	memset(proc->pdir, 0, PAGESIZE);

	// Copie du mapping kernel
	memcpy(proc->pdir, pgdir, 64 * sizeof(int));

	// alloc_map(proc->ssize, )
	// User stack allocation + mapping
	uint32_t stack_pages = proc->ssize / PAGESIZE;

	if (proc->ssize % PAGESIZE)
		stack_pages++;

	// printf("ustack -> %d\n", stack_pages);

	for (uint32_t i = 0; i < stack_pages; i++) {
		page = alloc_page();
		map_page(proc->pdir, page, (void*)(0x80000000 - (stack_pages - i) * PAGESIZE), P_USERSUP | P_RW);
	}

	proc->stack = (int*)page;
	// printf("ustack OK\n");

	// Kernel stack allocation
	proc->kstack = (uint32_t*)phys_alloc(2 * PAGESIZE);
	// proc->kstack = (uint32_t*)mem_alloc(2 * PAGESIZE);

	// Code allocation + mapping + copy
	const uint32_t code_size = (uint32_t)app->end - (uint32_t)app->start;
	uint32_t code_pages = code_size / PAGESIZE;

	if (code_size % PAGESIZE)
		code_pages++;

	// printf("code -> %d\n", code_pages);

	for (uint32_t i = 0; i < code_pages; i++) {
		page = alloc_page();
		// printf("page = %X\n", (int)page);
		map_page(proc->pdir, page, (void*)(0x40000000 + i * PAGESIZE), P_USERSUP);

		const uint32_t code_mod = code_size % PAGESIZE;
		const uint32_t size = (code_mod && i == code_pages-1) ? code_mod : PAGESIZE;

		memcpy(page, (void*)((uint32_t)app->start + i * PAGESIZE), size);
	}

	// printf("ucode OK\n");

	return success;
}
