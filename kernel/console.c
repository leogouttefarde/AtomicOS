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

void traitant_IT_33();

bool echo=true;
char tampon [TAILLE_TAMP];

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

char history[MAX_HISTORY][TAILLE_TAMP];

int hist_size = 0;
int hist_idx = -1;

static void afficher_echo(char car);


void history_add(char *string)
{
		// printf ("taille %s %d\n", string, strlen(string));
	if (hist_size == MAX_HISTORY-1) {
		memcpy((void*)((int)history + TAILLE_TAMP), history, sizeof(history) - TAILLE_TAMP);
		strcpy((char*)history, string);
	}
	else if (strlen(string) > 0) {
		strcpy((char*)&history[hist_size++][0], string);
	}

	hist_idx = -1;
}

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
/*
	// printf("cmd = %s\n", cmd);

	int len = strlen(cmd);


	// for (uint32_t i = 0; i < indice_ecr; i++) {
	// 	afficher_echo((char)127);
	// }
	int pos = indice_ecr - indice_lec;
	// printf("pos %d\n", (int)pos);
	int il = lig_cour();
	int ic = col_cour()-pos;
	place_curseur(il, ic);

	// for (uint32_t i = col_cour(); i < indice_ecr; i++) {
	// 	afficher_echo(' ');
	// }
	// place_curseur(il, ic);

	cases_dispos += len - pos;
	nb_lig = 0;
	indice_ecr -= pos;

	if (len > 0) {
		for (int i = 0; i < len+1; i++) {
			tampon[indice_ecr] = cmd[i];
			avancer(&indice_ecr);
		}
	}

	tampon[TAILLE_TAMP-1] = 0;

		// printf(cmd);
	for (int i = 0; i < len; i++) {
		afficher_echo(cmd[i]);
	}

	// place_curseur(il, ic);*/
}

static inline char *get_history(int i)
{
	// printf("idx = %d  s %d  i %d\n", hist_size-1 - i, hist_size, i);
	return (char*)&history[hist_size-1 - i];
}

void history_prev()
{
	char *cmd = NULL;

	if (0 < hist_size && hist_idx > 0) {
		cmd = get_history(--hist_idx);
	}
	else if (hist_idx == 0) {
		cmd = "";
		hist_idx--;
	}

	set_cmd(cmd);
}

void history_next()
{
	char *cmd = NULL;

	if (0 < hist_size && hist_idx < hist_size-1) {
		cmd = get_history(++hist_idx);
// printf("i %d next = %s\n", hist_idx, cmd);
	}
	set_cmd(cmd);
}

void cons_echo(int on)
{
	echo = on ? true : false;
}

void init_clavier(void)
{
	//Activation des interruptions claviers
	init_traitant_IT(33, (int)traitant_IT_33);
	masque_IRQ(1, false);
}

void traitant_clavier(void)
{
	outb(0x20, 0x20);
	char caractere=inb(0x60);
	do_scancode(caractere);
	debloque_io();
}

int cons_write(const char *str, long size)
{       
	console_putbytes(str, size);
	return 0;
}


unsigned long cons_read(char *string, unsigned long length)
{
	if (!length) return 0;

	/*Si aucune ligne complète n'a été tapée, 
	  le processus appelant est endormi*/
	while (!nb_lig) { 
		bloque_io();
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
			history_add(string);

			//Si on est sur un caractere 13
			nb_lig--;
			break;
		}
	}
	string[i] = '\0'; //Fin de ligne

	return i;
}

static void effacer_car_lig_cour() {
	char bs [3];

	bs[0]=8;
	bs[1]=32;
	bs[2]=8;
	console_putbytes(bs,3);
}

static void effacer_car_lig_prec() {
	place_curseur(lig_cour()-1,NB_COLS-1);
	console_putbytes(" ",1);
	place_curseur(lig_cour()-1,NB_COLS-1);
}

static void afficher_echo(char car)
{
	//Caractères affichés normalement
	if ((32 <= car && car <= 126) || car==9) {
		char chaine[1];
		chaine[0]=car;
		console_putbytes(chaine,1);
	}
	
	//caractères de contrôle
	else if (car<32 && car !=13) {
		char chaine [2];
		chaine[0]='^';
		// chaine[1] = 64+car;
		chaine[1] = 96 + car;
		console_putbytes(chaine,2);
	}
	else  {
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

void keyboard_data(char *str)
{
	const int len = strlen(str);


	// printf("taille : %lu\n", i);

	if (len < 0)
		return;

	// else if (len == 3) {
		// if (!strcmp(str, UP_ARROW)) {
		// 	history_next();
		// }
		// else if (!strcmp(str, DN_ARROW)) {
		// 	history_prev();
		// }
		// else if (!strcmp(str, LT_ARROW)) {

		// 	if (col_cour() > 0) {
		// 		place_curseur(lig_cour(), col_cour()-1);
		// 		reculer();
		// 	}
		// }
		// else if (!strcmp(str, RT_ARROW)) {

		// 	if (col_cour() < NB_COLS) {
		// 		place_curseur(lig_cour(), col_cour()+1);
		// 		avancer();
		// 	}
		// }
	// }
	else if (len == 1) {

		const char first = str[0];

		switch (first) {
		case 3:
			abort_shell_wait();
			break;

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
			// printf("_%X_\n", first);
			/* Autres cas : Si le tampon n'est pas plein, ajout
			   du caractere au tampon*/
			if (cases_dispos>0) {
				cases_dispos--;
				tampon[indice_ecr]=first;
				if (first==13) {
					nb_lig++;
					// wake_proc_waitio();
				}
				
				else if (first=='\t') {
					anc_lig[indice_ecr]=(col_cour()>=73);
					anc_col[indice_ecr]=col_cour();
				}
				avancer(&indice_ecr);                         
				if (echo)
					afficher_echo(first);
			}
			break;
		}
	}
}

// void kbd_leds(unsigned char leds)
// {
// 	printf("leds %d\n", leds);
//	// CAPS LOCK : 4
//	// NUM LOCK : 2
// }

;