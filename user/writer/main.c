
#include <atomic.h>
#include <stdio.h>
#include <stdbool.h>
#include <types.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <mem.h>

//L'initialisation de l'éditeur du texte
static void initWriter();
//L'affiche de la bannière de WRITER
static void printBannerWriter();

static void checkInputWriter();
//La sauvegarde du tampon de l'éditeur dans un fichier
static void saveBufferToFile();
//L'affiche du contenu du fichier s'il y en a et
//L'initialisation du tampon de l'éditeur
static void printFile_InitBuffer(const char *pathFile);
//La position Y (sur l'écran) du caractère se situant à la position donnée du tampon
int32_t positionY(int32_t position);
//La position X (sur l'écran) du caractère se situant à la position donnée du tampon
int32_t positionX(int32_t position);
//L'ajout d'un byte vide dans l'élement courant du tampon dé l'éditeur
void addByteToCurrentBuffer();
//La modification d'un byte dans l'élement courant du tampon dé l'éditeur
//et l'afficher
void modifyByteOnCurrentBuffer(char element); //position commence de 0
//Placer le curseur courant à son droite
void moveRight();
//Placer le curseur courant à sa gauche
void moveLeft();

typedef struct BufferElement_{
	char element;
	struct BufferElement_ *previous;
	struct BufferElement_ *next;
} BufferElement;

static int32_t bufferSize=0; //La taille actuelle du tampon de l'éditeur
static int32_t heightBanner=7; //La taille actuelle de la banière A MODIF

static BufferElement bufferHead={'0', NULL, NULL}; //sentinelle du tampon
static BufferElement *currentBuffer = &bufferHead; //l'élement courant du tampon
static int32_t currentPosBuffer = -1; //la position de l'élement courant du tampon
//static BufferElement *previousBuffer = NULL; //l'élement precédent du tampon
//static int32_t previousPosBuffer = -2; //la position de l'élement precédent du tampon

static char *fileName = NULL;

static int32_t width, height;

uint32_t time = 0;

int main(void)
{
	/*argc = 0;
	if (argc != 0){
		return -1;
		}*/

	printBannerWriter();
/*
	printf("\f");

	printf("\n%s\n",argv[0]);*/
	
	//L'initialisation de l'éditeur du texte
	initWriter();

	//Vérification de l'éxistence du fichier et l'affchier s'il existe
	//sinon le créer
	fileName = "test.txt";//argv[0];
	if (atomicExists(fileName)) {
		printFile_InitBuffer(fileName);
	}else{
		atomicClose(atomicOpen(fileName));
	}

	while(true){
		checkInputWriter();
	}

	return 0;
}

//L'initialisation de l'éditeur du texte
static void initWriter()
{
	//L'initialisation du chainage de la sentinelle du tampon
	bufferHead.element = 0;
	bufferHead.next = &bufferHead;
	bufferHead.previous = &bufferHead;

	set_cursor(heightBanner, 0);
        
	currentBuffer = &bufferHead;
	currentPosBuffer = -1;

	//appels systemes pour connaitre la dimension de l'écran
	width=getWidth();
	height=getHeight();
}

//L'affiche de la bannière de WRITER
static void printBannerWriter()
{
	const char *banner =
		" _       _    ______     _    _______    _____    ______    \n"\
		"| |     | |  |  __  \\   | |  |__   __|  |  ___|  |  __  \\   \n"\
		"| |  _  | |  | |__| |   | |     | |     | |___   | |__| |   \n"\
		"| | | | | |  |  __  /   | |     | |     |  ___|  |  __  /   \n"\
		"| |_| |_| |  | |  \\ \\   | |     | |     | |___   | |  \\ \\   \n"\
		"|_________|  |_|   \\_\\  |_|     |_|     |_____|  |_|   \\_\\  \n\n"
		;

	printf("\f");	
	
	cons_set_fg_color(LIGHT_CYAN);

	printf("%s", banner);

	cons_reset_color();
}

static void checkInputWriter()
{
	char input;
	do{
		//resetInputWriter();
		wait_clock(current_clock()+7);
	}while(cons_read(&input, 1)==0);
	
	//printf("-%c-", input);
	time++;

	//Entrée clavier pour sortir
	if(time==10)
	{
		saveBufferToFile();
		printf("\f");
		exit(0);
	}else{
		addByteToCurrentBuffer();
		moveRight();
		modifyByteOnCurrentBuffer(input);
	}

}

//La sauvegarde du tampon de l'éditeur dans un fichier
static void saveBufferToFile()
{
	File *fileToWrite = atomicOpen(fileName);

	char *buffer = mem_alloc(bufferSize * sizeof(char));

	BufferElement *current = &bufferHead;
	for(int32_t i=0; i<bufferSize; i++){
		buffer[i] = current->next->element;
		current = current->next;
	}

	atomicWrite(fileToWrite, buffer, sizeof(buffer));
}

//L'affiche du contenu du fichier s'il y en a et
//L'initialisation du tampon de l'éditeur
static void printFile_InitBuffer(const char *pathFile)
{
	File *fileToRead = atomicOpen(pathFile);
	
	char buffer; //le buffer à 1octet
	int32_t numBytesRead = 0; //nombre d'octet lu à chaque lecture

	while ((numBytesRead = atomicRead(fileToRead, &buffer, sizeof(char))) > 0) {
		cons_write(&buffer, numBytesRead);

		//L'ajout d'un caractère dans le tampon
		addByteToCurrentBuffer();
		moveRight();
		modifyByteOnCurrentBuffer(buffer);
	}

	atomicClose(fileToRead);
}

//La position Y (sur l'écran) du caractère se situant à la position donnée du tampon
int32_t positionY(int32_t position)
{
	return position % width;
}

//La position X (sur l'écran) du caractère se situant à la position donnée du tampon
int32_t positionX(int32_t position)
{
	int32_t posX = (position / width) + heightBanner;

	if(posX < height){
		return posX;
	}

	return posX; // A MODIF
	//scrollup(1);
	//printBannerWriter(); //A MODIF
	//set_cursor(height-1, positionY(currentPosBuffer));
}

//L'ajout d'un byte vide dans l'élement courant du tampon dé l'éditeur
void addByteToCurrentBuffer()
{
	BufferElement *newElement = mem_alloc(sizeof(BufferElement));
	if(newElement == NULL){
		exit(1); //on quitte l'éditeur s'il y a des erreurs d'allocation
	}
	bufferSize++;
	/*if(bufferSize>BUFFER_LIMITE){

	  }*/

	//Re-établir le chainage du tampon circulaire 
        //après l'insertion d'un élement
	BufferElement *nextBuffer = currentBuffer->next;
	currentBuffer->next = newElement;
	
	newElement->next = nextBuffer;
	newElement->previous = currentBuffer;

	nextBuffer->previous = newElement;
}

//Placer le curseur courant à son droite
void moveRight()
{
	currentBuffer = currentBuffer->next;
	currentPosBuffer++;
	set_cursor(positionX(currentPosBuffer), positionY(currentPosBuffer));
	//IF: A MODIF
}

//Placer le curseur courant à sa gauche
void moveLeft()
{
	currentBuffer = currentBuffer->previous;
	currentPosBuffer--;
	set_cursor(positionX(currentPosBuffer), positionY(currentPosBuffer));
	//IF: A MODIF
}

//La modification d'un byte dans l'élement courant du tampon dé l'éditeur
//et l'afficher
void modifyByteOnCurrentBuffer(char element)
{
	if(currentPosBuffer < 0 || currentPosBuffer >= bufferSize){
		return;
	}

	currentBuffer->element = element;

	//La modification sur l'écran
	set_caracter(positionX(currentPosBuffer), positionY(currentPosBuffer), 
		     currentBuffer->element);//A MODIF
}
