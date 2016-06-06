

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include "string.h"
#include "time.h"
#include "mem.h"
#include "file_handler.h"
#include "queue.h"

typedef struct File_ {

//struct parentDir;
//bool isFile;

	//le nom complet
	char *filename;
	uint32_t filenameLength;

	//que l'extension de nom
	char *extension;
	uint32_t extensionLength;

	uint32_t createdTime;
	uint32_t modifiedTime;

	uint32_t filesize;
	char *binaryData;

	//le chainage des fichiers dans un même répertoire
	struct File_ *next; 
} File;

typedef struct RootFileTable_{
	File *head;
	File *tail;
} RootFileTable;

//Le répertoire racine qui contient tous les fichiers créés
static RootFileTable rootDirectory = {NULL, NULL};

//La recherche naive du fichier identifié par son nom complet
static inline File *searchFile (RootFileTable *plist, const char* filename)
{
	if (plist == NULL || plist->head == NULL) {
		return NULL;
	}

	File *current = plist->head;

	//Parcours de la liste
	while (current != NULL && strcmp(filename, current->filename)!=0){
		current = current->next;
	}

	return current;
}

//La récupération du fichier identifié par son nom complet
//Cela peut servir à la suppresion et le déplacement du fichier
static inline File *extractFile (RootFileTable *plist, const char* filename)
{
	if (plist == NULL || plist->head == NULL) {
		return NULL;
	}

	File *current = plist->head;
	File *previous = NULL;

	while (current != NULL && strcmp(filename, current->filename)!=0){
		previous = current;
		current = current->next;
	}

	//Si l'élément cherché se trouve dans la tếte de la liste
	if(plist->head == current){
		plist->head = current->next;
		if (current == plist->tail) {
			plist->tail = NULL;
		}
	}else{//sinon
		if(plist->tail == current){
			plist->tail = previous;
		}
		previous->next = current->next;
	}

	current->next = NULL;
	return current;
}

//L'insertion du fichier dans la queue de la liste
static inline void insertFile (RootFileTable *plist, File *pfile)
{
	if (plist != NULL) {
		pfile->next = NULL;

		if (plist->tail != NULL) {
			plist->tail->next = pfile;  
		}

		plist->tail = pfile;  

		if (plist->head == NULL) {
			plist->head = plist->tail;
		}
	}
}

//La création d'un fichier
int createFile(const char *filename, uint32_t filesize)
{
	if(filename == NULL){
		return -1;
	}
	
	File *newFile = mem_alloc(sizeof(File));
	if(newFile == NULL){
		return -1;
	}

	//Recherche pour le doublon et le supprimer
	//Pour l'instant dans le repertoire racine A MODIF
	File *doublon = searchFile(&rootDirectory, filename);
	if (doublon!= NULL){
		deleteFile(filename);
	}

	newFile->filename = mem_alloc(strlen(filename)*sizeof(char));
	if(newFile->filename == NULL){
		mem_free_nolength(newFile);
		return -1;
	}
	strncpy(newFile->filename, filename, strlen(filename));
	newFile->filenameLength = strlen(filename);

	char *extension = strrchr(filename, '.');
	if(extension != NULL){
		extension++; //On oublie le '.'
		newFile->extension = mem_alloc(strlen(extension)*sizeof(char));
		//si erreur d'alloc => ce n'est pas grave, extension = NULL
		strncpy(newFile->extension, extension, strlen(extension));
		newFile->extensionLength = strlen(extension);
	}
	
	newFile -> createdTime = get_time();
	newFile -> modifiedTime = 0;

	newFile -> filesize = filesize;
	newFile -> binaryData = mem_alloc(filesize * sizeof(char));
	if(newFile->binaryData == NULL){
		mem_free_nolength(newFile->filename);
		mem_free_nolength(newFile);
		return -1;
	}

	newFile -> next = NULL;

	//Insertion du fichier dans la queue de la liste du repertoire designé
	//Pour l'instant dans le repertoire racine A MODIF
	insertFile(&rootDirectory, newFile);
	
	return 0;
}

//La suppression d'un fichier
int deleteFile (const char *filename)
{
	//Pour l'instant depuis le repertoire racine A MODIF
	File *fileToDelete = extractFile(&rootDirectory, filename);

	if (fileToDelete == NULL){
		return -1;
	}

	mem_free_nolength(fileToDelete->filename);
	mem_free_nolength(fileToDelete->extension);
	mem_free_nolength(fileToDelete->binaryData);
	mem_free_nolength(fileToDelete);
	
	return 0;
}

//Le renommage d'un fichier 
int renameFile(const char *filename1, const char *filename2)
{
	if(filename1 == NULL || filename2 == NULL){
		return -1;
	}

	//Recherche pour le fichier à renommer
	//Pour l'instant dans le repertoire racine A MODIF
	File *fileToRename = searchFile(&rootDirectory, filename1);
	if (fileToRename == NULL){
		return -1;
	}

	char *newName = mem_alloc(strlen(filename2)*sizeof(char));
	if(newName == NULL){
		return -1;
	}
	mem_free_nolength(fileToRename->filename);
	fileToRename->filename = newName;
	strncpy(fileToRename->filename, filename2, strlen(filename2));
	fileToRename->filenameLength = strlen(filename2);

	mem_free_nolength(fileToRename->extension);
	char *extension = strrchr(filename2, '.');
	if(extension != NULL){
		extension++; //On oublie le '.'
		fileToRename->extension = mem_alloc(strlen(extension)*sizeof(char));
		//si erreur d'alloc => ce n'est pas grave, extension = NULL
		strncpy(fileToRename->extension, extension, strlen(extension));
		fileToRename->extensionLength = strlen(extension);
	}
	
	fileToRename -> modifiedTime = get_time();
	
	return 0;
}

//L'affichage de nom de tous les fichiers
void listAllFiles()
{
	File *current = rootDirectory.head;

	printf("Contents in the root directory :");
	//Parcours de la liste pour afficher les fichiers
	uint32_t counter=1;
	while (current != NULL){
		printf("File (%d) : %s , %d Byte, created on %d", 
		       counter, current->filename, current->filesize, current->createdTime);
		counter++;
		current = current->next;
	}
}
