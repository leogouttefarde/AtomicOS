#include <atomic.h>
#include <stdio.h>
#include <stdbool.h>
#include <types.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define INITIAL_SIZE 5
#define INITIAL_STARTX 10
#define INITIAL_STARTY 10
#define INITIAL_LIFE 3
#define MAX_SIZE 100

#define RAND_MAX 32767 //pour le générateur aléatoire

//==============================================================================

typedef struct coordinates{
	int x;
	int y;
	int direction;
} Coordinates;

int start_column, start_line, width, height;
int snakeSize, currentSize, life;
int currentCurve = 0;
Coordinates fruit, snakeHead, snakeBody[MAX_SIZE], curve[MAX_SIZE];
int score = 0;

bool hasReachMax=false; //pour savoir si on a atteint la capacité max de tampon

//==============================================================================

//L'affiche de la bannière de SNAKE
static void printBannerSnake();
//Aller vers le haut de l'écran
static void goUp();
//Aller vers le bas de l'écran
static void goDown();
//Aller vers la gauche de l'écran
static void goLeft();
//Aller vers la droite de l'écran
static void goRight();
//L'affichage de la partie courbé du corps du serpent
static void getCurve();
//Generer un fruit
static void generateFruit();
//Une fonction élementaire pour tester l'égalité entres deux points
static bool testEqual(Coordinates p1, Coordinates p2);
//La création du fruit pour le serpent
//Le prolongement de la taille du serpent quand le serpent mange un fruit
static void getFruit();
//La construction de l'encadrement du jeu après l'effacement de l'écran
static void getBorder();
//L'initialisation des informations sur la tete du serpent
static void setSnakeHead(int x, int y, int direction);
//Renvoie true si on detecte une entrée clavier
static bool hasConsoleInput();
//L'initialisation des données sur le corps du serpent
static void reset_snakeBody();
//Ralentir le temps et l'affichage info sur le jeu
static void delayTime_info();
//Tester si le jeu est fini pour une vie du serpent
static void endGame();
//Pour faire bouger le serpent
static void getMoving();
//L'initialisation des paramètres du jeu
static void initGame();

//Composants du generateur aléatoire ; inspiré de osdev
static unsigned long int next = 1;
static int rand();
static void srand();


//==============================================================================

//La partie principale du jeu
int main()
{
	printBannerSnake();
	
	initGame();
	
	getBorder();

	srand();//pour initialiser le grain du générateur aléatoire

	getFruit();
	
	getMoving();//commencement du jeu

	return 0;
}

//L'initialisation des paramètres du jeu
static void initGame()
{
	/*L'initialisation de l'écran*/
	start_column = 0;
	start_line = 3;
	width=getWidth();
	height=getHeight();

	/*L'initialisation des paramètres liés au serpent*/
	snakeSize = INITIAL_SIZE;
	setSnakeHead(INITIAL_STARTX, INITIAL_STARTY, RIGHT);
	life=INITIAL_LIFE; //le nombre de vies supplémentaires
	curve[0]=snakeHead;
}

//L'affiche de la bannière de SNAKE
static void printBannerSnake()
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

	printf("\n\n Welcome to the Snake Mini-Game\n\n\n");

	printf("Game instructions :\n");
	printf("(1) Please use arrow keys to move the snake\n");
	printf("(2) Please escape key to quit the game\n\n");
	
	printf("Press any key to start the game !\n");
	wait_keyboard();

	cons_reset_color();
}

//Le dessin pour les fonctions 'go' selon la direction
static void helper_go(int i1, int i2, char symbol)
{
	if(currentSize==0){
		set_caracter(snakeHead.x + i1, snakeHead.y + i2, symbol);
	}else{
		set_caracter(snakeHead.x + i1, snakeHead.y + i2, '*');
	}
	snakeBody[currentSize].x=snakeHead.x + i1;
	snakeBody[currentSize].y=snakeHead.y + i2;
	currentSize++;
}

//Aller vers le haut de l'écran
static void goUp()
{
	for(int i=0; i<=(curve[currentCurve].x - snakeHead.x)&&currentSize<snakeSize; i++)
	{
		helper_go(i, 0, 'A');
	}

	getCurve();//dessiner la partie courbé du serpent

	if(!hasConsoleInput()){
		snakeHead.x--;
	}
}

//Aller vers le bas de l'écran
static void goDown()
{
	for(int i=0; i<=(snakeHead.x-curve[currentCurve].x)&&currentSize<snakeSize; i++)
	{
		helper_go(-1*i, 0, 'V');
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
		helper_go(0, i, '<');
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

//Le dessin
static void helper_getCurve(int i, int j1, int j2)
{

	snakeBody[currentSize].x=curve[i].x + j1;
	snakeBody[currentSize].y=curve[i].y + j2;
	set_caracter(snakeBody[currentSize].x, 
		     snakeBody[currentSize].y,
		     '*');
	currentSize++;
}

//L'affichage de la partie courbé du corps du serpent
static void getCurve()
{
	int difference;
	for(int i=currentCurve; i>=0&&currentSize<snakeSize; i--){
		if(i<0){
			if(hasReachMax){
				i=MAX_SIZE+i; //tampon mode circulaire
			}else{
				break;
			}
		}
		if(curve[i].y==curve[i-1].y){
			difference=curve[i].x-curve[i-1].x;
			if(difference < 0){
				for(int j=1; j<=(-1*difference); j++){
					helper_getCurve(i,j,0);
					if(currentSize==snakeSize){
						break;
					}
				}
			}else if(difference > 0){	
				for(int j=1; j<=difference; j++){
					helper_getCurve(i,-1*j,0);
					if(currentSize==snakeSize){
						break;
					}
				}
			}
		}else if(curve[i].x==curve[i-1].x){
			difference=curve[i].y-curve[i-1].y;
			if(difference < 0){
				for(int j=1; 
				    j<=(-1*difference) && currentSize<snakeSize; 
				    j++){
					helper_getCurve(i,0,j);
					if(currentSize==snakeSize){
						break;
					}
				}
			}else if(difference > 0){	
				for(int j=1; 
				    j<=difference && currentSize<snakeSize; 
				    j++){
					helper_getCurve(i,0,-1*j);
					if(currentSize==snakeSize){
						break;
					}
				}
			}
		}
	}
}

//Generer un fruit
static void generateFruit()
{
	fruit.x=rand()%(height-3);
	if(fruit.x<=start_line+1){
		fruit.x += (start_line+2);
	}
		
	fruit.y=rand()%(width-3);
	if(fruit.y<=start_column+1){
		fruit.y += (start_column+2);
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
		if (snakeSize<MAX_SIZE-1){
			snakeSize+=5;
		}
		score+=5;
		generateFruit();
	}else if(fruit.x==0){
		generateFruit();
	}
	cons_set_fg_color(LIGHT_RED);
	set_caracter(fruit.x, fruit.y, 'o');
	cons_reset_color();
}

//La construction de l'encadrement du jeu après l'effacement de l'écran
static void getBorder()
{
	printf("\f");
	cons_set_fg_color(LIGHT_GREEN);
	for(int i=start_line+1; i<height-1; i++){
		set_caracter(i, start_column, '|');
		set_caracter(i, width-1, '|');
	}

	for(int j=start_column+1; j<width-1; j++){
		set_caracter(start_line, j, '_');
		set_caracter(height-1, j, '_');
	}
	cons_reset_color();
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
	if (testInputGame()==1){
		return true;
	}
	
	return false;
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

//Ralentir le temps et l'affichage info sur le jeu
static void delayTime_info() // A MODIF
{
	cons_set_fg_color(LIGHT_CYAN);
	set_cursor(0, 10);
	printf("LIFE: %d",life);
	set_cursor(0, 25);
	printf("SCORE: %d",score);
	cons_reset_color();
	for(long long i=0; i<=(50000000); i++);
	//wait_clock(((unsigned long)(current_clock))+1); // A MODIF
}

//Tester si le jeu est fini pour une vie du serpent
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
		if(life>0){
			setSnakeHead(INITIAL_STARTX, INITIAL_STARTY, RIGHT);
			currentCurve=0;
			getMoving();
		}else{
			printf("\f");
			printf("\n\n ..... GAME OVER ..... YOUR SCORE WAS %d ....\n\n", score);
			exit(0); //Fin du jeu
		}
	}
}

//Pour faire bouger le serpent
static void getMoving()
{
	do{
		getFruit();

		resetInputGame();

		currentSize=0;

		reset_snakeBody();

		delayTime_info();

	        getBorder();

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
			break;
		}
		
		endGame();
		
	}while(!hasConsoleInput());

	int input = getInputGame();

	if(input==QUIT)
	{
		printf("\f");
		exit(0);
	}

	//La modification de la direction du serpent si une entrée clavier valide
	//est detecté
	if((input==RIGHT&&snakeHead.direction!=LEFT&&snakeHead.direction!=RIGHT)||
	   (input==LEFT&&snakeHead.direction!=RIGHT&&snakeHead.direction!=LEFT)||
	   (input==UP&&snakeHead.direction!=DOWN&&snakeHead.direction!=UP)||
	   (input==DOWN&&snakeHead.direction!=UP&&snakeHead.direction!=DOWN)){
		currentCurve++;
		if(currentCurve==MAX_SIZE){
			currentCurve%=MAX_SIZE; //tampon mode circulaire
			hasReachMax = true;
	        }
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
			break;
		}
	}
	
	getMoving();	
}

//==============================================================================

//Composants du generateur aléatoire ; inspiré de osdev
static int rand()
{
	next = next * 1103515245 + 12345;
	return (unsigned int)(next / 65536) % (RAND_MAX+1);
}
static void srand()
{
	//grain du generateur est basé de l'horloge
	unsigned long int seed;
	seed = (unsigned long int) current_clock();
	if (seed == 0){
		seed++;
	}
	next = seed;
}
