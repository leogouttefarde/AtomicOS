
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

	// Check if the PD entry is present
	if (pd[pdindex]) {
		uint32_t *pt = (uint32_t*)(pd[pdindex] & ~0xFFF);

		// Check if the PT entry is present
		if (pt[ptindex]) {
			return (void*)((pt[ptindex] & ~0xFFF) + ((uint32_t)virtualaddr & 0xFFF));
		}
	}

	return NULL;
}

/*
 * Vérifie la validité d'une adresse
 */
static inline bool is_valid_page(void *ptr)
{
	const uint32_t addr = (uint32_t)ptr;

	if (!addr || addr > 0xFFFFF000 || addr % PAGESIZE) {
		return false;
	}

	return true;
}

// Maps a virtual address to a physical address
bool map_page(uint32_t *pd, void *physaddr, void *virtualaddr, uint16_t flags)
{
	// Check page address validity
	if (pd == NULL || !is_valid_page(physaddr)
		|| !is_valid_page(virtualaddr))
		return false;

	uint32_t pdindex = (uint32_t)virtualaddr >> 22;
	uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

	// If the PD entry is not present, create it
	if (!pd[pdindex]) {

		// Copy the User\Supervisor flag + make the page table Read/Write
		uint16_t pdir_flags = (flags & P_USERSUP) | P_RW;
		void *page = alloc_page();

		assert(page);
		if (page != NULL) {
			memset(page, 0, PAGESIZE);
			pd[pdindex] = ((uint32_t)page) | (pdir_flags & 0xFFF) | P_PRESENT;
		}
	}

	uint32_t *pt = (uint32_t*)(pd[pdindex] & ~0xFFF);

	// If the PT entry is not present, map it
	if (!pt[ptindex]) {
		pt[ptindex] = ((uint32_t)physaddr) | (flags & 0xFFF) | P_PRESENT;

		// Flush the entry in the TLB
		tlb_flush();

		return true;
	}

	return false;
}

bool unmap_vpage(uint32_t *pd, void *virtualaddr)
{
	// Check page address validity
	if (pd == NULL || !is_valid_page(virtualaddr))
		return false;

	uint32_t pdindex = (uint32_t)virtualaddr >> 22;
	uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

	// Ensure the PD entry is present
	if (!pd[pdindex]) {
		return false;
	}

	uint32_t *pt = (uint32_t*)(pd[pdindex] & ~0xFFF);

	// If the PT entry is present, unmap it
	if (pt[ptindex]) {
		pt[ptindex] = (uint32_t)NULL;

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

	proc->pdir = (uint32_t*)alloc_page();

	if (proc->pdir == NULL)
		return false;

	// Initialisation du page directory
	memset(proc->pdir, 0, PAGESIZE);

	// Copie du mapping kernel
	memcpy(proc->pdir, pgdir, 64 * sizeof(int));

	// User stack allocation + mapping
	const uint32_t spages = compute_pages(proc->ssize);
	proc->spages = spages;

	for (uint32_t i = 0; i < spages; i++) {
		page = alloc_page();
		success &= map_page(proc->pdir, page,
			get_ustack_vpage(i, spages), P_USERSUP | P_RW);
	}

	proc->stack = (uint32_t*)page;

	// Kernel stack allocation
	proc->kstack = (uint32_t*)phys_alloc(KERNELSSIZE);

	if (proc->kstack == NULL)
		return false;

	// Code allocation + mapping + copy
	const uint32_t code_size = (uint32_t)app->end - (uint32_t)app->start;
	const uint32_t cpages = compute_pages(code_size);
	proc->cpages = cpages;

	for (uint32_t i = 0; i < cpages; i++) {
		page = alloc_page();
		success &= map_page(proc->pdir, page,
				get_ucode_vpage(i), P_USERSUP | P_RW);

		const uint32_t code_mod = code_size % PAGESIZE;
		const uint32_t size = (code_mod && i == cpages-1) ?
						code_mod : PAGESIZE;

		memcpy(page, (void*)((uint32_t)app->start + i * PAGESIZE), size);
	}

	return success;
}

