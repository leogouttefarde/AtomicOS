#include "interrupts.h"
#include "screen.h"
#include <stdint.h>
#include <stdbool.h>
#include <cpu.h>
#include <string.h>
#include "kbd.h"
#include "process.h"

#define MAX_HISTORY 20
#define TAILLE_TAMP 100

#define NONE 0
#define QUIT 1
#define UP 2
#define DOWN 3
#define LEFT 4
#define RIGHT 5

void traitant_IT_33();

bool echo=true;
char tampon [TAILLE_TAMP];
int inputGame=0; 

/*Tableaux servant à memoriser les positions des cararacteres
  à l'écran avant les tabulations*/
bool anc_lig[TAILLE_TAMP]; //vrai si la ligne est celle d'au dessus
uint8_t anc_col[TAILLE_TAMP];

unsigned long cases_dispos=TAILLE_TAMP; //cases dans lesquelles on peut écrire
unsigned long indice_ecr=0;
unsigned long indice_lec=0;
unsigned long nb_lig=0; //Nombre d'apparitions du caractère 13 dans le tampon

const char UP_ARROW[] = { 0x1B, 0x5B, 0x41, 0 };
const char DN_ARROW[] = { 0x1B, 0x5B, 0x42, 0 };
const char RT_ARROW[] = { 0x1B, 0x5B, 0x43, 0 };
const char LT_ARROW[] = { 0x1B, 0x5B, 0x44, 0 };

bool autocomp = false;
bool tab_pressed = false;
bool up=false;
bool down = false;

static void afficher_echo(char car);

//Avancer d'une case sur le tampon circulaire
static void avancer(unsigned long *i)
{
	*i=(*i+1)%TAILLE_TAMP;
}

//Reculer d'une case sur le tampon circulaire
static void reculer(unsigned long *i)
{
	if (*i > 0) 
		*i=*i-1;
	else 
		*i=TAILLE_TAMP-1;
}

void set_cmd(char *cmd)
{
	if (cmd == NULL) {
		return;
	}
}

void cons_echo(int on)
{
	echo = on ? true : false;
}

void cons_complete (int on) 
{
	autocomp = on ? true : false;
}

void init_clavier(void)
{
	//Activation des interruptions claviers
	init_traitant_IT(33, (int)traitant_IT_33);
	masque_IRQ(1, false);
}

void traitant_clavier(void)
{
	char caractere=inb(0x60);
	do_scancode(caractere);
	outb(0x20, 0x20);
	debloque_io();
}

int cons_write(const char *str, long size)
{       
	console_putbytes(str, size);
	return 0;
}

void clear_line() {
	long unsigned int prec = (indice_ecr > 0) ? indice_ecr -1 : TAILLE_TAMP-1;
	while (tampon[prec] != 13) {
		
		char c[] ={127,0};
		bool anc_echo = echo;
		
		if (tampon[prec] == (char) 252 || tampon[prec] == (char) 254 || tampon[prec] == 127) {
			echo=false;
		}
		keyboard_data(c);
		echo=anc_echo;
		prec = (prec > 0) ? prec -1 : TAILLE_TAMP-1;
	}
}
unsigned long cons_look (char *string, unsigned long length, unsigned char car) 
//On regarde les caractères dans le tampon, sans les consommer
{
	unsigned long i=0;
	unsigned long indice_lec_sauv = indice_lec;
	while (i<length) {
		
		if (tampon[indice_lec]!=car) {
			string [i]=tampon[indice_lec];
			i++;
		}
		else
			break;    

		
		avancer(&indice_lec);
	}
	
	indice_lec = indice_lec_sauv;

	//On supprime le dernier caractère tapé
	char c [] = {127,0};
	bool anc_echo = echo;
	echo=false;
	keyboard_data(c);
	echo=anc_echo;

	string[i] = car;
	return i;
}

unsigned long cons_read(char *string, unsigned long length)
{
	if (!length) return 0;

	/*Si aucune ligne complète n'a été tapée, 
	  le processus appelant est endormi*/
	while (! (nb_lig ||tab_pressed || up || down)) { 
		bloque_io();
	}

	//Cas de l'autocomplétion
	if (tab_pressed) {
		unsigned long ret = cons_look(string, length, '\t');
		tab_pressed = false;
		return ret;
	}

	//Cas d'un appui sur up
	if (up) {	
		unsigned long ret=cons_look(string, length,252);
		up=false;
		return ret;
	}

	if (down) {	
		unsigned long ret=cons_look(string, length,254);
		down=false;
		return ret;
	}

	bool fin=false; /*Indique si on le dernier caractère était 13*/
	unsigned long i=0;
 

	while (i<length) {
		//Parcours du tampon

		if (tampon[indice_lec]!=13) {
			string [i]=tampon[indice_lec];
			i++;
		}
		else
			fin=true;                
		
		cases_dispos++;
		avancer(&indice_lec);

		if (fin) {
			//Si on est sur un caractere 13
			nb_lig--;
			break;
		}
	}
	string[i] = '\0'; //Fin de ligne
	return i;
}


static void effacer_car_lig_cour() {
	char bs []= {8,32,8};
	console_putbytes(bs,3);
}

static void effacer_car_lig_prec() {
	place_curseur(lig_cour()-1,NB_COLS-1);
	console_putbytes(" ",1);
	place_curseur(lig_cour()-1,NB_COLS-1);
}

static void afficher_echo(char car)
{
	if (car<0)
		return;

	//Caractères affichés normalement
	if ((32 <= car && car <= 126) || car==9) {
		char chaine[] = {car};
		console_putbytes(chaine,1);
	}
	
	//caractères de contrôle
	else if (car<32 && car !=13) {
		char chaine [] = {'^', 96 + car};		
		console_putbytes(chaine,2);
	}

	else {
		switch (car) {
			char chaine[1];
			char bsctrl [6];
		case 13 : 
			//caractère appui sur entrée
			chaine[0]=10;
			console_putbytes(chaine,1);
			break;

		case 127:  
			//caractère backspace
			if (tampon[indice_ecr]<0 || tampon[indice_ecr]==127)
				return;

			if (tampon[indice_ecr]=='\t') {
				//Le caractere à effacer est une tabulation
				if (anc_lig[indice_ecr]) {
					place_curseur(lig_cour()-1,
						      anc_col[indice_ecr]);
				}
				else {
					place_curseur(lig_cour(),
						      anc_col[indice_ecr]);
				}

			}
			else if (tampon[indice_ecr]>=32) {
				//Le caractere à effacer est un caractere normal
				if (col_cour()==0 && lig_cour()>0) {
					//Il se situe sur la ligne precedente
					effacer_car_lig_prec();
					
				}
				else {
					//Il se situe sur la ligne actuelle
					effacer_car_lig_cour();
				}
			}

			else {
				//le caractere à effacer est un caractere de ctl
				
				if (col_cour()==0 && lig_cour()>0) {
					//Il se situe sur la ligne precedente
					place_curseur(lig_cour()-1,NB_COLS-2);
					console_putbytes("  ",2);
					place_curseur(lig_cour()-1,NB_COLS-2);
				}
				
				else if (col_cour()==1 && lig_cour()>0) {
					//Il se situe à cheval sur 2 lignes
					effacer_car_lig_cour();
					effacer_car_lig_prec();

				}
				
				else {
					//Il se situe sur la ligne actuelle
					bsctrl[0]=8;
					bsctrl[1]=8;
					bsctrl[2]=32;
					bsctrl[3]=32;
					bsctrl[4]=8;
					bsctrl[5]=8;
					console_putbytes(bsctrl,6);
				}                                        

			}
			break;
		default:
			break;
			
		}
		
	}
}

void ecrire_clavier (char *str) {

	for (unsigned int i=0; i < strlen(str); i++) {
		char c []={str[i],0};
		keyboard_data(c);
	}		
}

void keyboard_data(char *str)
{
	int len = strlen(str);
	char first = str[0];

	if (len < 0)
		return;

	if (len == 3) {
		if (!strcmp(str, UP_ARROW)) {
			up=true;
			len=1;
			first=252;
			inputGame = UP;
		}
		else if (!strcmp(str, DN_ARROW)) {
			down=true;
			len=1;
			first=254;
			inputGame = DOWN;
		}

		else if (!strcmp(str, LT_ARROW)) {
			inputGame = LEFT;
		}

		else if (!strcmp(str, RT_ARROW)) {
			inputGame = RIGHT;
		}
	}
	
	if (len == 1) {
		switch (first) {

		// CTRL + C
		case 3:
			abort_shell_wait();
			break;

		// CTRL + L
		case 12:
			scrollup(lig_cour());
			place_curseur(0, col_cour());
			break;

		// ESC
		case 27:
			inputGame = QUIT;
			break;

		// Backspace
		case 127:
			/*Cas du backspace : Si le tampon n'est pas vide 
			  l'indice recule d'une case*/
			if (cases_dispos!=TAILLE_TAMP) {
				reculer(&indice_ecr);
				cases_dispos++;
				if (echo)
					afficher_echo(first);                
			}
			break;

		default:
			/* Autres cas : Si le tampon n'est pas plein, ajout
			   du caractere au tampon*/
			if (cases_dispos>0) {
				cases_dispos--;
				tampon[indice_ecr]=first;
				
				if (first==13)
					nb_lig++;				
				
				else if (first=='\t') {
					anc_lig[indice_ecr]=(col_cour()>=72);
					anc_col[indice_ecr]=col_cour();
					if (autocomp) {
						tab_pressed=true;
						avancer(&indice_ecr);
						return;
					}

				}
				avancer(&indice_ecr);                         
				if (echo)
					afficher_echo(first);
			}
			break;
		}
		

	}
}

void reset_InputGame(void)
{
	inputGame = NONE;
}

int test_InputGame(void)
{
	if (inputGame!=NONE){
		return 1;
	}
	
	return 0;
}

int get_InputGame(void)
{
	up=false;
	down=false;
	return inputGame;
}


void wait_for_keyboard(void)
{
	//Tant qu'on n'a pas appuyé le clavier
	//le processus appelant est endormi
	while (cases_dispos==TAILLE_TAMP) { 
		bloque_io();
	}
}
