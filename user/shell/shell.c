#include <sysapi.h>
#include <stdbool.h>

#define TAILLE_TAB 2000

unsigned long debut_mot;
unsigned long fin_mot;
unsigned long fin_commande;
char commande[TAILLE_TAB];
extern unsigned long strtoul(const char *p, char **out_p, int base);

static void ecrire_msg (const char *message) {
	//Ecriture d'un message d'erreur sur la console
	cons_write(message,strlen(message));
}

static bool comparer(const char *mot_courant, const char *nom_commande) {
	//comparaison de 2 chaines
	return (strcmp(nom_commande,mot_courant) == 0);
}

static char *extraire_mot () {

	//Repérage du début du mot
	while (debut_mot<=fin_commande) {
		if (commande[debut_mot]==' ' || commande[debut_mot]=='\0')
			debut_mot++;
		else 
			break;
	}

	unsigned long indice_debut = debut_mot;

	if (debut_mot <= fin_commande) {
		//Cas où le mot n'est pas vide
	
		//Repérage de la fin du mot
		fin_mot = debut_mot+1;
		while (fin_mot<=fin_commande) {
			if (commande[fin_mot] != ' ')
				fin_mot++;
			else
				break;
		}

		commande[fin_mot] = '\0';
		debut_mot=fin_mot; //On place le debut du prochain mot
	}
		
	return &(commande[indice_debut]);

}

static void echo () {
	char *mot_courant = extraire_mot();
	char *mot_suivant = extraire_mot();
	
	if (mot_suivant[0] == '\0') {
		if (comparer(mot_courant,"on")) {
			cons_echo(1);
			return;
		}
		else if (comparer(mot_courant,"off")) {
			cons_echo(0);
			return;
		}
	}
	ecrire_msg("Cette commande necessite un unique \
argument : \"on\" ou \"off\"\n");
}

static void kill_proc () {
	char *mot_courant = extraire_mot();
	if (mot_courant[0]=='\0' || extraire_mot()[0]!='\0') {
		ecrire_msg("Cette commande necessite un unique argument \
: le pid du processus a tuer\n");
		return;
	}

	char *p;
	unsigned long numero=strtoul(mot_courant, &p,10);
	if (kill(numero) < 0) 
		ecrire_msg("pid invalide\n");
	else
		ecrire_msg("terminaison reussie\n");
	
}

static void ps() {
	if (extraire_mot()[0]=='\0')
		affiche_etats();
	else {
		char *message="Cette commande ne necessite pas d'arguments\n";
		cons_write(message,strlen(message));
	}	
		
}

static void sortie() {
	if (extraire_mot()[0]=='\0')
		exit(getpid());
	else {
		char *message="Cette commande ne necessite pas d'arguments\n";
		cons_write(message,strlen(message));
	}	
		
}


static void interpreter () {
	char *mot_courant=extraire_mot();
	if (comparer(mot_courant,"echo"))
		echo();		
	else if (comparer(mot_courant,"ps"))
		ps();
	else if (comparer(mot_courant,"kill"))
		kill_proc();
	else if (comparer(mot_courant,"exit"))
		sortie();
	else if (!comparer(mot_courant,""))
		cons_write("commande introuvable\n",21);
}


int main () {
	while(true) {
		cons_write(">",1);
		debut_mot=0;
		fin_commande = cons_read(commande,TAILLE_TAB)-1;
		interpreter(commande);
	}
	return 0;
}
