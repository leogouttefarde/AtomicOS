#include <atomic.h>
#include <stdio.h>
#include <stdbool.h>
#include <types.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "mem.h"

#define TAILLE_TAB 2000
#define TAILLE_HISTO 50

unsigned long debut_mot;
unsigned long fin_mot;
unsigned long fin_commande;
char commande[TAILLE_TAB];

char *histo[TAILLE_HISTO];

char *noms_commandes []={"autotest", "banner", "clear", "echo", "exit"
			 ,"help","history" ,"kill", "ps", "reboot","sleep", 
				   "snake", "test" ,"vesa"};
int plus_recent = -1;
unsigned int nb_histo=0;

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

	printf("This command requires an argument : argument : 'on' or 'off'\n");
}

static inline char *get_argument()
{
	char *mot_courant = extraire_mot();

	if (!mot_courant[0] || extraire_mot()[0]) {
		printf("This command requires an additional argument\n");

		return NULL;
	}

	return mot_courant;
}

static inline int parse_hex(char *str)
{
	return strtol(str, NULL, 16);
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
	printf("%s",cmd);
	printf(" : ");
	cons_reset_color();

	printf("%s",usage);
	printf("\n");
}

static void history() {
	unsigned int comm = plus_recent; 
	for (unsigned int i = 0; i < nb_histo; i++) {
		printf ("%i %s\n", i+1, histo[comm]);
		comm = (comm > 0) ? comm -1 : TAILLE_HISTO-1;		
	}
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
	cmd_usage("         vesa", "Test graphical mode");
	cmd_usage("        snake", "Play mini game SNAKE");
}

static bool interpreter ()
{
	bool error = false;
	int child = -1;
	char *mot_courant = extraire_mot();	

	if (compare(mot_courant, "echo")) {
		echo();
	}

	else if (compare(mot_courant, "ps")) {
		affiche_etats();
	}

	else if (compare(mot_courant, "kill")) {
		kill_proc();
	}

	else if (compare(mot_courant, "exit")) {
		if (getppid() > 0)
			return false;
	}

	else if (compare(mot_courant, "help")) {
		usage();
	}
	else if (compare(mot_courant, "history")) {
		history();
	}

	else if (compare(mot_courant, "sleep")) {

		char *next = get_argument();

		if (next != NULL) {
			sleep(parse_int(next));
		}
	}

	else if (compare(mot_courant, "reboot")) {
		reboot();
	}

	else if (compare(mot_courant, "clear")) {
		printf("\f");
	}

	else if (compare(mot_courant, "vesa")) {
		set_video_mode();
	}

	else if (compare(mot_courant, "display")) {
		char *next = get_argument();

		if (next != NULL) {
			bool loop;
			set_video_mode();
			
			loop = display(next);

			while (loop) {
				wait_clock(current_clock() + 3);

				loop &= getInputGame() != QUIT;
			}

			init_vbe_mode(0x3);
			print_banner();
		}
	}

	else if (compare(mot_courant, "vbe")) {
		char *next = get_argument();

		if (next != NULL) {
			init_vbe_mode(parse_hex(next));
		}
	}

	else if (compare(mot_courant, "vesamodes")) {
		char *arg0 = get_argument();
		// char *arg1 = get_argument();

		int min = 1000;
		int max = 4000;

		if (arg0 != NULL) {
			min = parse_int(arg0);
		}

		get_video_modes(min, max);
	}

	else if (compare(mot_courant, "autotest")) {
		child = start("autotest", 4000, 42, NULL);
	}

	else if (!strncmp(mot_courant, "test", 4)) {
		child = start(mot_courant, 4000, 128, NULL);

		if (child < 0) {
			error = true;
		}
	}

	else if (compare(mot_courant, "snake")) {

		// 1MB stack
		child = start("snake", 1024*4*1024, 42, NULL);
	}

	else if (!strncmp(mot_courant, "banner", 4)) {
		if (no_arguments()) {
			print_banner();
		}
	}

	else if (strlen(mot_courant)) {
		error = true;
	}

	if (error) {
		printf("%s : command not found\n", mot_courant);
	}

	if (child > 0) {
		waitpid(child, NULL);
	}

	return true;
}

static unsigned int nb_commandes_possibles(bool afficher, int *numero_commande) {

	int nb_commandes = sizeof(noms_commandes)/sizeof(char *);
	int res = 0;
	for (int i =0; i < nb_commandes; i++) {
		if ( strlen(noms_commandes[i]) >= fin_commande) {
			bool est_candidat =true;
			for (unsigned int j = 0; j < fin_commande; j++) {
				if (noms_commandes[i][j] != commande[j]) {
					est_candidat=false;
					break;
				}
			}
			if (est_candidat) {
				*numero_commande = i;
				res++;
				if (afficher)
					printf ("%s\n",noms_commandes[i]);
			}
		}
	}
	return res;
}

static void afficher_msg (int reaf) {
	if (reaf == -1)
		return;
	cons_set_fg_color(LIGHT_CYAN);
	printf("AtomicOS>");
	cons_reset_color();
	if (reaf==1) {
		for (unsigned int i = 0; i <fin_commande; i++)
			printf("%c",commande[i]);
	}
}

int reafficher=0;
int post_hist=-1; 
unsigned int fleches_consec=0;
int autocompleter() {
	int num_commande;
	unsigned int nb = nb_commandes_possibles(false,&num_commande);
	if (nb==1) {
		unsigned int taille_nom = strlen(noms_commandes[num_commande]);
		for (unsigned int i = fin_commande; i < taille_nom; i ++) {
			char ch[2];
			ch[0]=noms_commandes[num_commande][i];
			ch[1]=0;	
			keyboard_data(ch);
		}
		return -1;
	}

	else if (nb > 0) {
		printf ("\n");
		nb_commandes_possibles(true,&num_commande);		
		return 1;
	}
	else {
		return -1;
	}
	
}

int main()
{
	bool execute = true;

	while (execute) {
		afficher_msg(reafficher);

		debut_mot = 0;
		fin_commande = cons_read(commande,TAILLE_TAB);

		if (commande[fin_commande]=='\t') {
			commande[fin_commande]='\0';
			reafficher=autocompleter();
		}

		else if (commande[fin_commande]==(char)252) {

			if (fleches_consec + 1 <= nb_histo) {
				if (post_hist==-1) {
					post_hist = plus_recent;
				}
				else {
					
					post_hist = (post_hist > 0) ? post_hist -1 : TAILLE_HISTO-1;
				}
				fleches_consec ++;
				clear_line();
				char c[strlen(histo[post_hist])+1];
				unsigned int i=0;
				while ( i < strlen(histo[post_hist])) {
					c[i] = histo[post_hist][i];
					i++;
				}
				c[i] = 0;
				keyboard_data(c);
			}
			reafficher=-1;
		}
		else if (commande[fin_commande]==(char)254) {
			if (post_hist != -1) {
				clear_line();

				if (post_hist != plus_recent) {
					//printf("DONE");
					post_hist = (post_hist +1) % TAILLE_HISTO;
					char c[strlen(histo[post_hist])+1];
					//char c[(int)histo[post_hist]+1];
					unsigned int i=0;
				while ( i < strlen(histo[post_hist])) {
					c[i] = histo[post_hist][i];
					i++;
				}
				c[i] = 0;
				keyboard_data(c);
					/*for (unsigned int i = 0; i < strlen(histo[post_hist]); i ++) {
						char c[] ={histo[post_hist][i],0} ;
						//printf("%s",c);
						keyboard_data(c);
						}*/
					
				}
				else  {					
					post_hist=-1;
				}
				fleches_consec --;

			}
			reafficher=-1;
			
		}
			
		else if (fin_commande > 0){
			
			fin_commande--;
			execute = interpreter(commande);

			plus_recent = (plus_recent + 1) % TAILLE_HISTO;

			if (nb_histo == TAILLE_HISTO) {
				//printf("%u",strlen(histo[plus_recent]));
				//mem_free(histo[plus_recent],2+strlen(histo[plus_recent]));
				mem_free(histo[plus_recent],1+strlen(histo[plus_recent]));
			}
			else {
				nb_histo++;
			}
			histo[plus_recent] = mem_alloc(fin_commande+2);
			memcpy(histo[plus_recent],commande,fin_commande+2);

			fleches_consec=0;
			reafficher=0;
			post_hist=-1;
		}
		else {
			fleches_consec=0;
			reafficher=0;
			post_hist=-1;
		}
	}

	return 0;
}
