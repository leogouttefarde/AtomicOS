/*****************************************************
 * Copyright Léo Gouttefarde 2015                    *
 *           Jean-Baptiste Boric 2015                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#ifndef __LISTE_ZL_H
#define __LISTE_ZL_H

#ifndef min
#define min(a, b) (((a)<(b))?(a):(b))
#endif

/*
 * Nombre de tailles de bloc non utilisées (1, 2 et 4)
 */
#define NB_UNUSED_SIZES 3


/*
 * Structure de bloc libre
 */
typedef struct zone_libre {
    struct zone_libre *suiv;
} Zone_libre;

/*
 * Structure de liste de blocs
 */
typedef struct liste_zl {
    Zone_libre *tete;
    Zone_libre *queue;
} Liste_zl;


/*
 * Ajoute une zone libre à la liste donnée
 */
void liste_zl_add(Liste_zl *liste, void *zone_libre);

/*
 * Retire la zone libre en tête de liste et la renvoie
 */
void *liste_zl_pop(Liste_zl *liste);

/*
 * Recherche une zone libre précise dans une liste,
 * puis la retire et la renvoie si présente.
 */
void *liste_zl_pop_elem(Liste_zl *liste, void *zone_libre);


#endif
