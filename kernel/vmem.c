
#include "vmem.h"
#include "apps.h"
#include "mem.h"
#include <stddef.h>
#include <string.h>

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

static inline uint32_t compute_pages(uint32_t size)
{
	uint32_t pages = size / PAGESIZE;

	if (size % PAGESIZE)
		pages++;

	return pages;
}


bool alloc_pages(Process *proc)
{
	bool success = true;
	void *page = NULL;

	if (proc == NULL)
		return false;

	struct uapps *app = get_app(proc->name);

	if (app == NULL)
		return false;

	// printf("starting %s\n", app->name);

	proc->pdir = (uint32_t*)alloc_page();
	// printf("proc->pdir -> %X\n", (uint32_t)proc->pdir);

	if (proc->pdir == NULL)
		return false;

	// Initialisation du page directory
	memset(proc->pdir, 0, PAGESIZE);

	// Copie du mapping kernel
	memcpy(proc->pdir, pgdir, 64 * sizeof(int));

	// alloc_map(proc->ssize, )
	// User stack allocation + mapping
	const uint32_t spages = compute_pages(proc->ssize);
	proc->spages = spages;

	// printf("ustack -> %d\n", stack_pages);

	for (uint32_t i = 0; i < spages; i++) {
		page = alloc_page();
		success &= map_page(proc->pdir, page,
			get_ustack_vpage(i, spages), P_USERSUP | P_RW);
	}

	proc->stack = (uint32_t*)page;
	// printf("ustack OK\n");

	// Kernel stack allocation
	proc->kstack = (uint32_t*)phys_alloc(KERNELSSIZE);

	if (proc->kstack == NULL)
		return false;

	// Code allocation + mapping + copy
	const uint32_t code_size = (uint32_t)app->end - (uint32_t)app->start;
	const uint32_t cpages = compute_pages(code_size);
	proc->cpages = cpages;
	// printf("code -> %d\n", code_pages);

	for (uint32_t i = 0; i < cpages; i++) {
		page = alloc_page();
		// printf("page = %X\n", (int)page);
		success &= map_page(proc->pdir, page,
				get_ucode_vpage(i), P_USERSUP | P_RW);

		const uint32_t code_mod = code_size % PAGESIZE;
		const uint32_t size = (code_mod && i == cpages-1) ?
						code_mod : PAGESIZE;

		memcpy(page, (void*)((uint32_t)app->start + i * PAGESIZE), size);
	}

	// printf("ucode OK\n");

	return success;
}

