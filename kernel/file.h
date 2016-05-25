
#ifndef __FILE_H__
#define __FILE_H__

//crée une file de messages
int pcreate(int count);

//détruit une file de messages
int pdelete(int fid);

//dépose un message dans une file
int psend(int fid, int message);

//retire un message d'une file
int preceive(int fid,int *message);

//réinitialise une file
int preset(int fid);

//renvoie l'état courant d'une fil
int pcount(int fid, int *count);

#endif
