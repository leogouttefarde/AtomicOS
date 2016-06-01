
#ifndef __CLAVIER_H__
#define __CLAVIER_H__

void init_clavier();
unsigned long cons_read(char *string, unsigned long length);
void cons_echo(int on);
int cons_write(const char *str, long size);

//Effacer le buffer de l'entrée clavier pour le jeu
void reset_InputGame(void);

//Tester l'existence d'une entrée clavier pour le jeu
int test_InputGame(void);

//Renvoyer l'entrée clavier pour le jeu la plus récente
int get_InputGame(void);

#endif
