
#include "liste_zl.h"
#include <stdio.h>
#include <stddef.h>


/*
 * Ajoute une zone libre à la liste donnée
 */
void liste_zl_add(Liste_zl *liste, void *zone_libre)
{
    if (liste && zone_libre) {
        Zone_libre *zone = zone_libre;

        zone->suiv = NULL;

        if (liste->queue) {
            liste->queue->suiv = zone;
            liste->queue = liste->queue->suiv;
        }
        else {
            liste->tete = zone;
            liste->queue = zone;
        }
    }
}

/*
 * Retire la zone libre en tête de liste et la renvoie
 */
void *liste_zl_pop(Liste_zl *liste)
{
    Zone_libre *pop = NULL;

    if (liste && liste->tete) {
        pop = liste->tete;
        liste->tete = pop->suiv;

        if (!liste->tete)
            liste->queue = NULL;
    }

    return pop;
}

/*
 * Recherche une zone libre précise dans une liste,
 * puis la retire et la renvoie si présente.
 */
void *liste_zl_pop_elem(Liste_zl *liste, void *zone_libre)
{
    if (zone_libre && liste) {
        Zone_libre *zl = liste->tete;
        Zone_libre *prev = zl;

        /* On regarde si l'élément à retirer est présent */
        while (zl && zl != zone_libre){
            prev = zl;
            zl = zl->suiv;
        }

        /* On retire l'élément de la liste */
        if (zl == zone_libre) {
            prev->suiv = zl->suiv;

            if (zl == liste->queue) {
                if (zl == prev)
                    liste->queue = NULL;

                else
                    liste->queue = prev;
            }

            if (zl == liste->tete)
                liste->tete = zl->suiv;

            return zl;
        }
    }

    return NULL;
}
