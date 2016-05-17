#include "interruptions.h"
#include "screen.h"
#include <stdbool.h>
#include <cpu.h>
#include "kbd.h"

void traitant_IT_33();
/*
void init_clavier(void) {
        init_traitant_IT(33, traitant_IT_33);
        masque_IRQ(1,false);
}*/

void traitant_clavier(void) {
        outb(0x20, 0x20);
        char caractere=inb(0x60);
        do_scancode(caractere);
}

void keyboard_data(char *str) {
        int i=0;
        while (str[i]!=0)   {
                //Cas du retour à la ligne
                if (str[i]==13) {
                        console_putbytes("\n",1);
                }

                //Cas du retour arrière
                else if (str[i]==127) {
                                char bs [3];
                                bs[0]=8;
                                bs[1]=32;
                                bs[2]=8;
                                console_putbytes(bs,3);
                }
                else {
                        console_putbytes(&(str[i]),1);
                }
                i++;
        }
}
