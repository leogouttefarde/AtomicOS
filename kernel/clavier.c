#include "interruptions.h"
#include "screen.h"
#include <stdint.h>
#include <stdbool.h>
#include <cpu.h>
#include "kbd.h"

#define TAILLE_TAMP 2000

bool echo=true;

void traitant_IT_33();
char tampon [TAILLE_TAMP];
unsigned long cases_dispos=TAILLE_TAMP;
int indice_ecr=0;
int indice_lec=0;
int nb_lig=0;


void cons_echo(int on) {
        echo=(on==0) ? false : true;
}

void init_clavier(void) {
        init_traitant_IT(33, traitant_IT_33);
        masque_IRQ(1,false);
}

void traitant_clavier(void) {
        outb(0x20, 0x20);
        char caractere=inb(0x60);
        do_scancode(caractere);
}


int cons_write(const char *str, long size) {       
        console_putbytes(str,size);
        return 0;
}


unsigned long cons_read(char *string, unsigned long length){
        if (length==0) return 0;

        /*Si aucune ligne complète n'a été tapée, 
          le processus appelant est endormi*/
        //pas encore implémenté
        while (nb_lig==0) {
                
        }


        bool fin=false; /*Indique si on le dernier caractère était un \n*/
        unsigned long i=0;
        unsigned long max = (length>TAILLE_TAMP-cases_dispos) ? 
                TAILLE_TAMP-cases_dispos : length;

        //Copie du tampon dans string
        while (i<max) {


                if (tampon[indice_lec]!=13) {
                        string [i]=tampon[indice_lec];
                        i++;
                }
                else {
                        fin=true;
                }
                
                cases_dispos++;
                indice_lec=(indice_lec+1) % TAILLE_TAMP;
                if (fin) break;
        }
        if (fin) nb_lig--;
        return i;
        
}


static void afficher_echo(char car) {
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
                chaine[1]=96+car;
                console_putbytes(chaine,2);
        }
        else  {
                switch (car) {
                        char chaine[1];
                        char bs [3];
                case 13 : 
                        //caractère saut de ligne
                        chaine[0]=10;
                        console_putbytes(chaine,1);
                        break;
                      
                case 127:  
                        //caractère backspace
                        //A COMPLETER
                        bs[0]=8;
                        bs[1]=32;
                        bs[2]=8;
                        console_putbytes(bs,3);
                        break;
                default:
                        break;
                        
                }
                
        }
}

void keyboard_data(char *str) {
        int i = 0;
        while (str[i]!=0)   {                

                switch (str[i]) {
                case 127:
                        if (indice_ecr > 0) {
                                indice_ecr--;
                        }
                        else {
                                indice_ecr=TAILLE_TAMP-1;
                        }
                        
                        if (cases_dispos<TAILLE_TAMP) {
                                cases_dispos++;
                        }
                        break;
                default:
                        //si le tampon n'est pas plein
                        if (cases_dispos>0) {
                                cases_dispos--;
                                tampon[indice_ecr]=str[i];
                                indice_ecr=(indice_ecr+1) % TAILLE_TAMP;
                                if (str[i]==13) nb_lig++;
                        }
                        break;
                }
                
                if (echo) {
                        afficher_echo(str[i]);
                }
                i++;
        }

}
int lancer_console (void *p) {
        if (p==0){} 
        sti(); //A mettre ici ?
        while (true) {
                char commandes [2] [80];
                init_clavier();
                cons_read(commandes[0],80);
                cons_read(commandes[1],80);
                cons_write(commandes[0],80);
                cons_write(commandes[1],80);
        }
        return 0;
}
