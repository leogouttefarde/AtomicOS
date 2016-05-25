#include <stddef.h>
#include <stdint.h>
#include "userspace_apps.h"
#include "hash.h"

// Table de hachage
hash_t table;

void init_apps()
{
	// Initialisation de la table
	hash_init_string(&table);

	// Insertion des symboles
	for (uint32_t i = 0; symbols_table[i].start != NULL; i++) {
		hash_set(&table, (void*)symbols_table[i].name,
			(void*)&symbols_table[i]);
	}
}

// Renvoie l'adresse du début de la zone mémoire de l'application
// Si l'application n'est pas trouvée, renvoie NULL
struct uapps *get_app(const char *name)
{
	struct uapps *app = NULL;

	if (name != NULL) {
		app = (struct uapps*)hash_get(&table, (void*)name, NULL);
	}

	return app;
}

void free_apps()
{
	hash_destroy(&table);
}
