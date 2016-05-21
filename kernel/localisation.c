#include "userspace_apps.h"
#include "stdint.h"
#include "hash.h"

#define TAILLE_TABLE 100

hash_t table ={0,0,TAILLE_TABLE,0,0,0};

void init_hash_table() {
	//Insertion des symboles dans la table de hash

	hash_init_string(&table);
	uint32_t i=0;

	while (symbols_table[i].start!=0) {
		hash_set(&table,(void *)symbols_table[i].name,
			 (void *) &(symbols_table[i]));
		i++;
	}

}

void *chercher_hash_table (const char *nom) {
	//Renvoie l'adresse du début de la zone mémoire de l'application
	//Si l'application n'est pas trouvée, renvoie 0
	void * appli=hash_get(&table,(void *)nom,0);
	return (appli) ? (*(struct uapps*)appli).start : 0;
}
