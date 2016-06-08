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
#define MAX_SIZE 105 //La longueur max du serpent 
                     //Doit etre multiple de DELTA_LENGTH et 
                     //y ajoute INITIAL_SIZE
#define DELTA_SCORE 10 //La valeur d'incrementation de score
#define DELTA_LENGTH 5 //La valeur d'incrementation de longueur

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
int score = 0;

Coordinates fruit, snakeHead, snakeBody[MAX_SIZE], curve[MAX_SIZE];

bool hasReachMax=false; //pour savoir si on a atteint la capacité max de tampon
bool endSet=false; 

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
//La création du fruit pour le serpent au début ou après avoir mangé
//Le prolongement de la taille du serpent quand le serpent mange un fruit
//jusqu'à la limite de la taille
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
//Pour faire bouger le serpent jusqu'il tourne ou jusqu'à la partie est finie
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
	
	while(true){
		clear_line();
		getMoving();//commencement & continuité du jeu
	}
	return 0;
}

//L'initialisation des paramètres du jeu
static void initGame()
{
	/*L'initialisation de l'écran*/
	start_column = 0;
	start_line = 3;
	//appels systemes pour connaitre la dimension de l'écran
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
	printf("(1) Use arrow keys to move the snake\n");
	printf("(2) Use the ESC key to quit the game\n\n");
	
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

//Le dessin pour la partie courbée du serpent
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
	int i=currentCurve;
	while(currentSize<snakeSize){
		int current = i;
		int previous = i-1;
		if(previous < 0){
			if(hasReachMax){//tampon mode circulaire
				previous = MAX_SIZE - 1;
				i = MAX_SIZE;
			}else{
				break;
			}
		}

		if(curve[current].y==curve[previous].y){
			difference=curve[current].x-curve[previous].x;
			if(difference < 0){
				for(int j=1; j<=(-1*difference); j++){
					helper_getCurve(current,j,0);
					if(currentSize==snakeSize){
						break;
					}
				}
			}else if(difference > 0){	
				for(int j=1; j<=difference; j++){
					helper_getCurve(current,-1*j,0);
					if(currentSize==snakeSize){
						break;
					}
				}
			}
		}else if(curve[current].x==curve[previous].x){
			difference=curve[current].y-curve[previous].y;
			if(difference < 0){
				for(int j=1; 
				    j<=(-1*difference) && currentSize<snakeSize; 
				    j++){
					helper_getCurve(current,0,j);
					if(currentSize==snakeSize){
						break;
					}
				}
			}else if(difference > 0){	
				for(int j=1; 
				    j<=difference && currentSize<snakeSize; 
				    j++){
					helper_getCurve(current,0,-1*j);
					if(currentSize==snakeSize){
						break;
					}
				}
			}
		}

		i--;
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

//La création du fruit pour le serpent au début ou après avoir mangé
//Le prolongement de la taille du serpent quand le serpent mange un fruit
//jusqu'à la limite de la taille
static void getFruit()
{
	if(testEqual(snakeHead, fruit)){
		if (snakeSize<MAX_SIZE-1){
			snakeSize+=DELTA_LENGTH;
		}
		score+=DELTA_SCORE;
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
static void delayTime_info()
{
	cons_set_fg_color(LIGHT_CYAN);
	set_cursor(0, 10);
	printf("LIFE: %d",life);

	set_cursor(0, 25);
	printf("SCORE: %d",score);
	cons_reset_color();

	//On ralentit le jeu pour avoir une meilleur expérience de jouer
	wait_clock(current_clock()+7);
}

//Tester si le jeu est fini pour une vie du serpent
static void endGame()
{
	//Pour savoir si le serpent touche son corps
	bool isTouch = false;
	for(int i=4; i<snakeSize; i++){
		if(testEqual(snakeBody[0], snakeBody[i])){
			isTouch=true;
			break;
		}
	}

	//Si le serpent touche le mur ou son corps
	if(snakeHead.x <= start_line || snakeHead.x >= height || snakeHead.y <= start_column ||
	   snakeHead.y >= width || isTouch){
		life--;
		if(life>0){
			setSnakeHead(INITIAL_STARTX, INITIAL_STARTY, RIGHT);
			currentCurve=0;
			curve[0]=snakeHead;
			hasReachMax=false;
			endSet = true;
		}else{
			printf("\f");
			printf("\n\n ..... GAME OVER ..... YOUR SCORE WAS %d ....\n\n", score);
			exit(0); //Fin du jeu
		}
	}
}

//Pour faire bouger le serpent jusqu'qu'il tourne ou jusqu'à la partie est finie
static void getMoving()
{
	//On continue dans la même direction tant qu'il n'y a pas d'entrée clavier
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
		
		//Pour savoir si le jeu est fini complètement ou juste une partie
		endGame();
		if(endSet){
			endSet=false;
			return;
		}
		
	}while(!hasConsoleInput());

	//Une entrée clavier detectée
	int input = getInputGame();
	
	//Entrée clavier pour sortir
	if(input==QUIT)
	{
		printf("\f");
		exit(0);
	}

	//La modification de la direction du serpent si une entrée clavier 
	//de direction valide est detecté
	if((input==RIGHT && snakeHead.direction!=LEFT && snakeHead.direction!=RIGHT)||
	   (input==LEFT && snakeHead.direction!=RIGHT && snakeHead.direction!=LEFT)||
	   (input==UP && snakeHead.direction!=DOWN && snakeHead.direction!=UP)||
	   (input==DOWN && snakeHead.direction!=UP && snakeHead.direction!=DOWN)){
		currentCurve++;
		if(currentCurve == MAX_SIZE){
			currentCurve %= MAX_SIZE; //tampon mode circulaire
			hasReachMax = true;
	        }
		curve[currentCurve] = snakeHead;
		snakeHead.direction = input;

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
