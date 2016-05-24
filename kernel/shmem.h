
#ifndef __SHMEM_H__
#define __SHMEM_H__


void shm_init();

/**
 * Permet au processus appelant de demander la création
 * d'une page partagée au noyau, identifiée par la chaine key.
 */
 // Si la page est allouée et mappé, son adresse virtuelle est retournée.
 // En cas d'erreur (key est nulle, la page existe déjà, plus de mémoire), NULL est retourné.
void *shm_create(const char *key);

/**
 * Permet à un processus d'obtenir une référence
 * sur une page partagée connue du noyau sous le nom key.
 * Si la page est disponible, elle est mappée pour ce processus
 * et son adresse est retournée. Sinon, la fonction retourne NULL.
 */
void *shm_acquire(const char *key);

/**
 * Informe le noyau que le processus appelant veut relacher la référence
 * sur la page nommée par key.
 * Si une référence a été acquise (par appel à shm_acquire par exemple),
 * la page est démappée et l'adresse virtuelle fournie précédement
 * n'est plus valide, sinon l'appel est sans effet.
 * Quand cet appel amène à relacher la dernière référence sur une page
 * partagée, la page physique correspondante est effectivement libérée.
 */
void shm_release(const char *key);

void shm_free();

#endif
