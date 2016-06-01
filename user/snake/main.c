#include <atomic.h>
#include <stdio.h>
#include <stdbool.h>
#include <types.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define UP 72
#define DOWN 80
#define LEFT 75
#define RIGHT 77

#define INITIAL_SIZE 5
#define INITIAL_STARTX 20
#define INITIAL_STARTY 20
#define INITIAL_LIFE 3
#define MAX_SIZE 30

//==============================================================================

typedef struct coordinates{
	int x;
	int y;
	int direction;
} Coordinates;

int start_column, start_line, width, height;
int snakeSize, currentSize, life;
int currentCurve = 0;
Coordinates fruit, snakeHead, snakeBody[MAX_SIZE], curve[500];

//==============================================================================

//L'affiche de la bannière de SNAKE
void printBannerSnake();
//Aller vers le haut de l'écran
static void goUp();
//Aller vers le bas de l'écran
static void goDown();
//Aller vers la gauche de l'écran
static void goLeft();
//Aller vers la droite de l'écran
static void goRight();
//L'affichage de tous les parties courbées du serpent
static void getCurve();
//Generateur aléatoire
static int rand_generator();
//Generer un fruit
static void generateFruit();
//Une fonction élementaire pour tester l'égalité entres deux points
static bool testEqual(Coordinates p1, Coordinates p2);
//La création du fruit pour le serpent
//Le prolongement de la taille du serpent quand le serpent mange un fruit
static void getFruit();
//La construction de l'encadrement du jeu
static void getBorder();
//L'initialisation des informations sur la tete du serpent
static void setSnakeHead(int x, int y, int direction);
//Renvoie true si on detecte une entrée clavier
static bool hasConsoleInput();
//Renvoie le dernier caractère dans le buffer du clavier
static int getConsoleInput();
//L'initialisation des données sur le corps du serpent
static void reset_snakeBody();
//Ralentir le temps
static void delayTime();
//Tester si le jeu est fini // A MODIF
static void endGame();
//Pour faire bouger le serpent
static void getMoving();

//==============================================================================

//La partie principale du jeu
int main()
{
	start_column = 5;
	start_line = 2;
	width=70;
	height=50;

	printBannerSnake();
	
	snakeSize = INITIAL_SIZE;

	setSnakeHead(INITIAL_STARTX, INITIAL_STARTY, RIGHT);
	
	getBorder();

	getFruit();

	life=INITIAL_LIFE; //le nombre de vies supplémentaires

	curve[0]=snakeHead;
	
	getMoving();

	return 0;
}

//L'affiche de la bannière de SNAKE
void printBannerSnake()
{
	const char *banner =
" _________    __________    __________    ___   ____    ________  \n"\
"|    _____|  |    __    |  |    __    |  |   | /   /   |    ____| \n"\
"|   |_____   |   |  |   |  |   |  |   |  |   |/   /    |   |____  \n"\
"|_____    |  |   |  |   |  |   |__|   |  |    _   \\    |    ____| \n"\
" _____|   |  |   |  |   |  |    __    |  |   | \\   \\   |   |____  \n"\
"|_________|  |___|  |___|  |___|  |___|  |___|  \\___\\  |________| \n\n"
;

	printf("\f");	
	
	cons_set_fg_color(LIGHT_CYAN);

	printf("%s", banner);

	printf("Welcome to the Snake game\n\n");

	printf("Game instructions :\n");
	printf("-> Use arrow keys to move the snake\n\n");
	
	printf("Press any key to start the game !\n");

	cons_reset_color();
}

//Aller vers le haut de l'écran
static void goUp()
{
	for(int i=0; i<=(curve[currentCurve].x - snakeHead.x)&&currentSize<snakeSize; i++)
	{
		if(currentSize==0){
			set_caracter(snakeHead.x + i, snakeHead.y, 'A');
		}else{
			set_caracter(snakeHead.x + i, snakeHead.y, '*');
		}
		snakeBody[currentSize].x=snakeHead.x + i;
		snakeBody[currentSize].y=snakeHead.y;
		currentSize++;
	}

	getCurve();

	if(!hasConsoleInput()){
		snakeHead.x--;
	}
}

//Aller vers le bas de l'écran
static void goDown()
{
	for(int i=0; i<=(snakeHead.x-curve[currentCurve].x)&&currentSize<snakeSize; i++)
	{
		if(currentSize==0){
			set_caracter(snakeHead.x - i, snakeHead.y, 'V');
		}else{
			set_caracter(snakeHead.x - i, snakeHead.y, '*');
		}
		snakeBody[currentSize].x=snakeHead.x - i;
		snakeBody[currentSize].y=snakeHead.y;
		currentSize++;
	}

	getCurve();

	if(!hasConsoleInput()){
		snakeHead.x++;
	}	
}

//Aller vers la gauche de l'écran
static void goLeft()
{
	for(int i=0; i<=(curve[currentCurve].y - snakeHead.y)&&currentSize<snakeSize; i++)
	{
		if(currentSize==0){
			set_caracter(snakeHead.x, snakeHead.y + i, '<');
		}else{
			set_caracter(snakeHead.x, snakeHead.y + i, '*');
		}
		snakeBody[currentSize].x=snakeHead.x;
		snakeBody[currentSize].y=snakeHead.y + i;
		currentSize++;
	}

	getCurve();

	if(!hasConsoleInput()){
		snakeHead.y--;
	}
}

//Aller vers la droite de l'écran
static void goRight()
{
	for(int i=0; i<=(snakeHead.y - curve[currentCurve].y)&&currentSize<snakeSize; i++)
	{
		snakeBody[currentSize].x=snakeHead.x;
		snakeBody[currentSize].y=snakeHead.y - i;

		if(currentSize==0){
			set_caracter(snakeHead.x, snakeHead.y - i, '>');
		}else{
			set_caracter(snakeHead.x, snakeHead.y - i, '*');
		}
		currentSize++;
	}

	getCurve();

	if(!hasConsoleInput()){
		snakeHead.y++;
	}
}

//L'affichage de tous les parties courbées du serpent
static void getCurve()
{
	int difference;
	for(int i=currentCurve; i>=0&&currentSize<snakeSize; i--){
		if(curve[i].y==curve[i-1].y){
			difference=curve[i].x==curve[i-1].x;
			if(difference < 0){
				for(int j=1; j<=(-1*difference); j++){
					snakeBody[currentSize].x=curve[i].x + j;
					snakeBody[currentSize].y=curve[i].y;
					set_caracter(snakeBody[currentSize].x, 
						     snakeBody[currentSize].y,
						     '*');
					currentSize++;
					if(currentSize==snakeSize){
						break;
					}
				}
			}else if(difference > 0){	
				for(int j=1; j<=difference; j++){
					snakeBody[currentSize].x=curve[i].x - j;
					snakeBody[currentSize].y=curve[i].y;
					set_caracter(snakeBody[currentSize].x, 
						     snakeBody[currentSize].y,
						     '*');
					currentSize++;
					if(currentSize==snakeSize){
						break;
					}
				}
			}
		}else if(curve[i].x==curve[i-1].x){
			difference=curve[i].y==curve[i-1].y;
			if(difference < 0){
				for(int j=1; j<=(-1*difference); j++){
					snakeBody[currentSize].x=curve[i].x;
					snakeBody[currentSize].y=curve[i].y + j;
					set_caracter(snakeBody[currentSize].x, 
						     snakeBody[currentSize].y,
						     '*');
					currentSize++;
					if(currentSize==snakeSize){
						break;
					}
				}
			}else if(difference > 0){	
				for(int j=1; j<=difference; j++){
					snakeBody[currentSize].x=curve[i].x;
					snakeBody[currentSize].y=curve[i].y - j;
					set_caracter(snakeBody[currentSize].x, 
						     snakeBody[currentSize].y,
						     '*');
					currentSize++;
					if(currentSize==snakeSize){
						break;
					}
				}
			}
		}
	}
}

//Generateur aléatoire
static int rand_generator()
{
	return 15; //A MODIF
}

//Generer un fruit
static void generateFruit()
{
	fruit.x=rand_generator()%height;
	if(fruit.x<=start_line){
		fruit.x += (start_line+1);
	}
		
	fruit.y=rand_generator()%width;
	if(fruit.y<=start_column){
		fruit.y += (start_column+1);
	}
}

//Une fonction élementaire pour tester l'égalité entres deux points
static bool testEqual(Coordinates p1, Coordinates p2)
{
	return (p1.x==p2.x && p1.y==p2.y);
}

//La création du fruit pour le serpent
//Le prolongement de la taille du serpent quand le serpent mange un fruit
static void getFruit()
{
	if(testEqual(snakeHead, fruit)){
		snakeSize++; //A MODIF
		//refaire le grain de generateur
		generateFruit();
	}else if(fruit.x==0){
		generateFruit();
	}
}

//La construction de l'encadrement du jeu
static void getBorder()
{
	printf("\f");
	for(int i=start_line+1; i<height-1; i++){
		set_caracter(i, start_column, '|');
		set_caracter(i, width, '|');
	}

	for(int j=start_column+1; j<width-1; j++){
		set_caracter(start_line, j, '_');
		set_caracter(height-1, j, '_');
	}
}

//L'initialisation des informations sur la tete du serpent
static void setSnakeHead(int x, int y, int direction)
{
	snakeHead.x = x;
	snakeHead.y = y;
	snakeHead.direction = direction;
}

//Renvoie true si on detecte une entrée clavier
static bool hasConsoleInput()
{
	return false; //A MODIF
}

//Renvoie le dernier caractère dans le buffer du clavier
static int getConsoleInput()
{
	return DOWN;// A MODIF
}

//L'initialisation des données sur le corps du serpent
static void reset_snakeBody()
{
	for(int i=0; i<MAX_SIZE; i++){
		snakeBody[i].x=0;
		snakeBody[i].y=0;
		
		if(i==snakeSize){
			break;
		}
	}
}

//Ralentir le temps
static void delayTime()
{
	for(long long i=0; i<=(10000000); i++);
}

//Tester si le jeu est fini // A MODIF
static void endGame()
{
	bool isTouch = false;
	for(int i=4; i<snakeSize; i++){
		if(testEqual(snakeBody[0], snakeBody[i])){
			isTouch=true;
			break;
		}
	}

	if(snakeHead.x <= start_line || snakeHead.x >= height || snakeHead.y <= start_column ||
	   snakeHead.y >= width || isTouch){
		life--;
		if(life>=0){
			setSnakeHead(INITIAL_STARTX, INITIAL_STARTY, RIGHT);
			getMoving();
		}else{
			printf("\f");
			printf("\n\n END OF GAME ..... \n\n");
			exit(0); //Fin du jeu
		}
	}
}

//Pour faire bouger le serpent
static void getMoving()
{
	do{
		getFruit();
		
		//flush_input_console()

		currentSize=0;

		reset_snakeBody();

		delayTime();

		getBorder(); // A MODIF necessaire?

		switch(snakeHead.direction){
		case LEFT:
			goLeft();
			break;
		case RIGHT:
			goRight();
			break;
		case UP:
			goUp();
			break;
		case DOWN:
			goDown();
			break;
		default:
			break; // A MODIF
		}
		
		endGame();
		
	}while(!hasConsoleInput());

	int input = getConsoleInput();

	if(input==27)
	{
		exit(0);
	}

	//La modification de la direction du serpent si une entrée clavier valide
	//est detecté
	if((input==RIGHT&&snakeHead.direction!=LEFT&&snakeHead.direction!=RIGHT)||
	    (input==LEFT&&snakeHead.direction!=RIGHT&&snakeHead.direction!=LEFT)||
	    (input==UP&&snakeHead.direction!=DOWN&&snakeHead.direction!=UP)||
	   (input==DOWN&&snakeHead.direction!=UP&&snakeHead.direction!=DOWN)){
		currentCurve++;
		curve[currentCurve]=snakeHead;
		snakeHead.direction=input;

		switch(input){
		case LEFT:
			snakeHead.y--;
			break;
		case RIGHT:
			snakeHead.y++;
			break;
		case UP:
			snakeHead.x--;
			break;
		case DOWN:
			snakeHead.x++;
			break;
		default:
			break; // A MODIF
		}
	}
	
	getMoving();	
}
