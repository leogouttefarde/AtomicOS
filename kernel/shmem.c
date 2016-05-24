#include <stddef.h>
#include <stdint.h>
#include "userspace_apps.h"
#include "hash.h"
#include "vmem.h"
#include "mem.h"
#include "string.h"

// On mappe les zones mémoires au dessus de la pile
#define SHMADDR USERSTACK

// Amélio possible : trier les sharedpage allouées par adresse virt
// et allouer à la première vaddr dispo
#define MAXPAGES 0x80000

#define NEXTPAGE(p) ((void*)(SHMADDR + p->shm_idx++ * PAGESIZE))
#define NAME_MAX 255

typedef struct SharedPage_ {
	void *key;
	void *page;
	uint32_t nrefs;
} SharedPage;

// Table de hachage
static hash_t table;

void shm_init()
{
	// Initialisation de la table
	hash_init_string(&table);
}

/**
 * Permet au processus appelant de demander la création
 * d'une page partagée au noyau, identifiée par la chaine key.
 *
 * Si la page est allouée et mappé, son adresse virtuelle est retournée.
 * En cas d'erreur (key est nulle, la page existe déjà, plus de mémoire),
 * NULL est retourné.
 */
void *shm_create(const char *key)
{
	void *page, *vpage = NULL;
	bool ret = false;

	if (key == NULL || hash_isset(&table, (void*)key)) {
		return NULL;
	}

	const uint32_t ksize = strlen(key);

	Process *proc = get_cur_proc();

	if (ksize >= NAME_MAX || proc == NULL)
		return NULL;

	page = alloc_page();

	if (page == NULL) {
		return NULL;
	}

	if (proc->shm_idx < MAXPAGES) {
		vpage = NEXTPAGE(proc);
		ret = map_page(proc->pdir, page, vpage, P_USERSUP | P_RW);
	}

	if (ret) {
		SharedPage *shp = mem_alloc(sizeof(SharedPage));

		if (shp != NULL) {

			// Copie de la clé
			shp->key = mem_alloc(ksize);

			shp->page = page;
			shp->nrefs = 1;

			if (!hash_set(&table, shp->key, (void*)shp)) {
				hash_set(&proc->shmem, shp->key, vpage);
			}
			else {
				ret = false;
			}
		}
		else {
			ret = false;
		}
	}

	if (!ret) {
		vpage = NULL;
		free_page(page);
	}

	return vpage;
}

/**
 * Permet à un processus d'obtenir une référence
 * sur une page partagée connue du noyau sous le nom key.
 * Si la page est disponible, elle est mappée pour ce processus
 * et son adresse est retournée. Sinon, la fonction retourne NULL.
 */
void *shm_acquire(const char *key)
{
	void *pkey = (void*)key;
	void *vpage = NULL;

	if (key == NULL || !hash_isset(&table, pkey))
		return NULL;

	Process *proc = get_cur_proc();

	if (proc == NULL)
		return NULL;

	if (!hash_isset(&proc->shmem, pkey) && proc->shm_idx < MAXPAGES) {
		SharedPage *shp = hash_get(&table, pkey, NULL);
		vpage = NEXTPAGE(proc);

		if (shp && map_page(proc->pdir, shp->page, vpage, P_USERSUP | P_RW)) {

			if (!hash_set(&proc->shmem, pkey, vpage)) {
				shp->nrefs++;
			}

			else {
				unmap_vpage(proc->pdir, vpage);
				vpage = NULL;
			}
		}
		else {
			vpage = NULL;
		}
	}

	return vpage;
}

/**
 * Informe le noyau que le processus appelant veut relacher la référence
 * sur la page nommée par key.
 * Si une référence a été acquise (par appel à shm_acquire par exemple),
 * la page est démappée et l'adresse virtuelle fournie précédement
 * n'est plus valide, sinon l'appel est sans effet.
 * Quand cet appel amène à relacher la dernière référence sur une page
 * partagée, la page physique correspondante est effectivement libérée.
 */
void shm_release(const char *key)
{
	void *pkey = (void*)key;

	if (key == NULL || !hash_isset(&table, pkey))
		return;

	Process *proc = get_cur_proc();

	if (proc == NULL)
		return;

	if (hash_isset(&proc->shmem, pkey)) {
		SharedPage *shp = hash_get(&table, pkey, NULL);
		void *vpage = hash_get(&proc->shmem, pkey, NULL);

		if (shp != NULL && vpage != NULL) {
			shp->nrefs--;

			unmap_vpage(proc->pdir, vpage);
			hash_del(&proc->shmem, pkey);

			if (shp->nrefs < 1) {
				hash_del(&table, pkey);

				free_page(shp->page);
				mem_free_nolength(shp->key);
				mem_free_nolength(shp);
			}
		}
	}
}

void shm_free()
{
	hash_destroy(&table);
}
