
#ifndef __FILE_HANDLER_H__
#define __FILE_HANDLER_H__

#include <stdint.h>

//La création d'un fichier vide
int createFile(const char *filename);

//La suppression d'un fichier
int deleteFile (const char *filename);

//Le renommage d'un fichier 
int renameFile(const char *filename1, const char *filename2);

//L'affichage de nom de tous les fichiers
void listAllFiles();

//La lecture de données binaires d'un fichier
//Interdit de modifier directement ces données binaires ("dataToRead")
int readBinaryData(const char *filename, char **dataToRead, 
		    uint32_t *sizeToRead);

//L'écriture de données binaires avec la mode écrasement dans un fichier
int writeBinaryData(const char *filename, char *dataToWrite, 
		     uint32_t sizeToWrite);

#endif
