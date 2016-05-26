
#ifndef __CLAVIER_H__
#define __CLAVIER_H__

void init_clavier();
unsigned long cons_read(char *string, unsigned long length);
void cons_echo(int on);
int cons_write(const char *str, long size);

#endif
