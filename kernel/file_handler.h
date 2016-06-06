
#ifndef __FILE_HANDLER_H__
#define __FILE_HANDLER_H__

#include <stdint.h>

//La création d'un fichier
int createFile(const char *filename, uint32_t filesize);

//La suppression d'un fichier
int deleteFile (const char *filename);

//Le renommage d'un fichier 
int renameFile(const char *filename1, const char *filename2);

//L'affichage de nom de tous les fichiers
void listAllFiles();

//int putBinaryData(const char *filename, const char *data);

//int modifyBinaryData(const char *filename, const char *newData);

#endif
