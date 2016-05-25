/*
 * Copyright (C) 2005 -- Simon Nieuviarts
 * Copyright (C) 2012 -- Damien Dejean <dam.dejean@gmail.com>
 *
 * Kernel and physical memory allocators.
 */
#include "mem.h"
#include "types.h"
#include "liste_zl.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// We manage physical memory between 64MB and 256MB = 192MB ~ 2^27.6
// Therefore the maximum buddy index is 27
#define BUDDY_MAX_INDEX 27

/* Available memory lists */
static Liste_zl tzl[BUDDY_MAX_INDEX + 1 - NB_UNUSED_SIZES];

// Physical memory
static void *zone_memoire = NULL;
extern char mem_end[];

/* Kernel heap boundaries */
extern char mem_heap[];
extern char mem_heap_end[];
static char *curptr = mem_heap;

/* Trivial sbrk implementation */
void *sbrk(ptrdiff_t diff)
{
	char *s = curptr;
	char *c = s + diff;
	if ((c < curptr) || (c > mem_heap_end)) return ((void*)(-1));
	curptr = c;
	return s;
}


/*
 * Gère l'accès aux différentes listes de zones libres
 */
static inline Liste_zl *get_tzl(uint8_t index)
{
    const uint8_t real_index = index - NB_UNUSED_SIZES;

    if (NB_UNUSED_SIZES <= index && index <= BUDDY_MAX_INDEX)
        return &tzl[real_index];

    return NULL;
}

/*
 * Découpe récursivement le premier bloc mémoire disponible à la bonne taille
 */
static inline void *reduce_block(uint8_t taille, uint8_t taille_pere)
{
    /* Découpe récursive du bloc père en buddies */
    if (taille_pere > taille) {
        Zone_libre *pere;
        Zone_libre *buddy[2];
        const uint8_t taille_fils = taille_pere - 1;

        pere = liste_zl_pop(get_tzl(taille_pere));

        if (pere) {
            buddy[0] = pere;
            buddy[1] = (void*)((uint32_t)pere + (1 << taille_fils));

            liste_zl_add(get_tzl(taille_fils), buddy[0]);
            liste_zl_add(get_tzl(taille_fils), buddy[1]);

            return reduce_block(taille, taille_fils);
        }
    }

    /* Pas de découpe à effectuer */
    else if (taille_pere == taille)
        return liste_zl_pop(get_tzl(taille));


    return NULL;
}

/*
 * Calcule l'adresse du buddy d'une zone
 */
static inline void *get_buddy(void *ptr, uint32_t taille)
{
    return zone_memoire + ((ptr - zone_memoire) ^ taille);
}

/*
 * Libère et retourne le buddy d'un pointeur, ou NULL si erreur
 */
static inline void *pop_buddy(void *ptr, uint32_t taille)
{
    if (taille > BUDDY_MAX_INDEX)
        return NULL;

    const uint32_t taille_mem = 1 << taille;
    Zone_libre *buddy = get_buddy(ptr, taille_mem);

    return liste_zl_pop_elem(get_tzl(taille), buddy);
}

/*
 * Calcul optimal de ceil(log2(size))
 */
static inline uint8_t next_log2(uint32_t size)
{
    uint32_t size2 = size;
    uint8_t nb = 0;

    while (size2 >>= 1)
        nb++;

    if (size != (uint32_t)(1 << nb))
        nb++;

    return nb;
}

/*
 * Calcule la puissance de 2 nécessaire pour
 * allouer un bloc de taille "size"
 */
static inline uint8_t get_block_size(uint32_t size)
{
    /* On s'assure que les zones aient toutes
     * assez de place pour leur gestion interne */
    if (size < sizeof(Zone_libre))
        size = sizeof(Zone_libre);

    return next_log2(size);
}

/*
 * Vérifie la validité d'une adresse allouée
 */
static inline bool valid_address(void *ptr)
{
    if (!zone_memoire || !ptr || ptr < zone_memoire
        || ptr >= (void*)mem_end) {
        // || ptr >= (zone_memoire + ALLOC_MEM_SIZE))
        return false;
    }

    return true;
}

int phys_init()
{
    if (zone_memoire == NULL)
        zone_memoire = mem_heap_end;

    if (zone_memoire == NULL) {
        // perror("mem_init:");
        return -1;
    }

    // Initialize the free memory block lists
    memset(tzl, 0, sizeof(tzl));

    // Add initial free memory blocks
    // First 64MB memory block
    liste_zl_add(get_tzl(BUDDY_MAX_INDEX - 1), zone_memoire);

    // Additionnal 128MB memory block
    liste_zl_add(get_tzl(BUDDY_MAX_INDEX), (zone_memoire + 0x4000000));

    return 0;
}

void *phys_alloc(unsigned long size)
{
    /* Gestion des erreurs */
    if (!zone_memoire || !size)
        return NULL;

    /* Calcul de la puissance de 2 à allouer */
    const uint8_t taille = get_block_size(size);

    void *mem = NULL;
    uint8_t cible = taille;

    /* Recherche du bloc mémoire de taille minimale à allouer */
    while (cible < BUDDY_MAX_INDEX && !get_tzl(cible)->tete)
        cible++;

    /* Si un bloc mémoire est disponible, on le découpe au maximum */
    if (cible <= BUDDY_MAX_INDEX && get_tzl(cible)->tete)
        mem = reduce_block(taille, cible);

    return mem;
}

int phys_free(void *ptr, unsigned long size)
{
    if (!valid_address(ptr))
        return -1;

    const uint8_t taille = get_block_size(size);
    void *buddy = pop_buddy(ptr, taille);

    /* Si pas de buddy trouvé, on ajoute simplement une zone libre */
    if (buddy == NULL)
        liste_zl_add(get_tzl(taille), (Zone_libre*)ptr);

    /* Sinon, on fusionne les buddies */
    else {
        // min génère une instruction non supportée en debug,
        // calcul manuel donc
        // void *fusion = min(ptr, buddy);
        void *fusion = buddy;

        if (ptr < buddy)
            fusion = ptr;

        phys_free(fusion, 1 << (taille + 1));
    }

    return 0;
}

int phys_destroy()
{
    zone_memoire = NULL;
    return 0;
}

