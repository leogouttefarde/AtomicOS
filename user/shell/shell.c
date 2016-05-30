#include <atomic.h>
#include <stdio.h>
#include <stdbool.h>
#include <types.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define TAILLE_TAB 2000

unsigned long debut_mot;
unsigned long fin_mot;
unsigned long fin_commande;
char commande[TAILLE_TAB];


static bool compare(const char *mot_courant, const char *nom_commande)
{
	if (mot_courant != NULL && nom_commande != NULL)
		return !strcmp(nom_commande, mot_courant);

	return false;
}

static char *extraire_mot ()
{
	//Repérage du début du mot
	while (debut_mot <= fin_commande) {
		if (commande[debut_mot] == ' ' || !commande[debut_mot])
			debut_mot++;
		else 
			break;
	}

	unsigned long indice_debut = debut_mot;

	if (debut_mot <= fin_commande) {
		// Cas où le mot n'est pas vide
	
		// Repérage de la fin du mot
		fin_mot = debut_mot+1;
		while (fin_mot <= fin_commande) {

			if (commande[fin_mot] != ' ')
				fin_mot++;
			else
				break;
		}

		commande[fin_mot] = '\0';
		debut_mot = fin_mot; //On place le debut du prochain mot
	}
		
	return &(commande[indice_debut]);
}

static void echo ()
{
	char *mot_courant = extraire_mot();
	char *mot_suivant = extraire_mot();
	
	if (!mot_suivant[0]) {

		if (compare(mot_courant, "on")) {
			cons_echo(1);
			return;
		}

		else if (compare(mot_courant, "off")) {
			cons_echo(0);
			return;
		}
	}

	printf("Cette commande necessite un unique "\
			"argument : \"on\" ou \"off\"\n");
}

static inline char *get_argument()
{
	char *mot_courant = extraire_mot();

	if (!mot_courant[0] || extraire_mot()[0]) {
		printf("Cette commande necessite un argument supplémentaire");

		return NULL;
	}

	return mot_courant;
}

static inline int parse_int(char *str)
{
	return strtol(str, NULL, 10);
}

static void kill_proc ()
{
	char *next = get_argument();

	if (next == NULL)
		return;

	const int pid = parse_int(next);

	if (pid != getpid() || getppid() > 0) {
			
		if (kill(pid) < 0)
			printf("Invalid process id\n");

		else
			printf("Process killed\n");
	}
}

static bool no_arguments()
{
	char *word = extraire_mot();

	if (!word[0]) {
		return true;
	}

	else {
		printf("This command has no arguments\n");
	}

	return true;
}


void cmd_usage(char *cmd, char *usage)
{
	cons_set_fg_color(LIGHT_CYAN);
	printf(cmd);
	printf(" : ");
	cons_reset_color();

	printf(usage);
	printf("\n");
}

void usage()
{
	cons_set_fg_color(LIGHT_CYAN);
	printf("AtomicOS Shell Commands :\n");
	cons_reset_color();

	cmd_usage("     autotest", "Execute all tests");
	cmd_usage("       banner", "Print the banner");
	cmd_usage("       testNN", "To execute a test from 1 to 22, type test1, test2, ..., test22");
	cmd_usage("        clear", "Clear the screen");
	cmd_usage("           ps", "Display process informations");
	cmd_usage("      echo on", "Enable keyboard input display");
	cmd_usage("     echo off", "Disable keyboard input display");
	cmd_usage("         exit", "Exit the current shell");
	cmd_usage("         help", "Display this help");
	cmd_usage("   kill <pid>", "Kill the corresponding process");
	cmd_usage("       reboot", "Reboot the computer");
	cmd_usage(" sleep <secs>", "Sleep for secs seconds");
}

static bool interpreter ()
{
	int child = -1;
	char *mot_courant = extraire_mot();

	if (compare(mot_courant, "echo")) {
		echo();
	}

	else if (compare(mot_courant, "ps")) {
		if (no_arguments())
			affiche_etats();
	}

	else if (compare(mot_courant, "kill")) {
		kill_proc();
	}

	else if (compare(mot_courant, "exit")) {
		if (no_arguments() && getppid() > 0)
			return false;
	}

	else if (compare(mot_courant, "help")) {
		usage();
	}

	else if (compare(mot_courant, "sleep")) {

		char *next = get_argument();

		if (next != NULL) {
			sleep(parse_int(next));
		}
	}

	else if (compare(mot_courant, "reboot")) {
		if (no_arguments())
			reboot();
	}

	else if (compare(mot_courant, "clear")) {
		if (no_arguments()) {
			printf("\f");
		}
	}

	else if (compare(mot_courant, "autotest")) {
		if (no_arguments()) {
			child = start("autotest", 4000, 42, NULL);
		}
	}

	else if (!strncmp(mot_courant, "test", 4)) {
		if (no_arguments()) {
			child = start(mot_courant, 4000, 128, NULL);
		}
	}

	else if (!strncmp(mot_courant, "banner", 4)) {
		if (no_arguments()) {
			print_banner();
		}
	}

	else if (!compare(mot_courant, "")) {
		cons_write("commande introuvable\n",21);
	}

	if (child > 0) {
		waitpid(child, NULL);
	}

	return true;
}

/*
help
time (commande affichant le nombre de tics depuis le début)
CTRL+C to stop current prog

complétion commandes

historique commandes : reprendre code tp shell
*/

int main()
{
	bool execute = true;

	while (execute) {
		cons_set_fg_color(LIGHT_CYAN);
		printf("AtomicOS>");
		cons_reset_color();

		debut_mot = 0;
		fin_commande = cons_read(commande,TAILLE_TAB);

		if (fin_commande > 0) {
			fin_commande--;
			execute = interpreter(commande);
		}
	}

	return 0;
}
