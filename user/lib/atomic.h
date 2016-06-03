
#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#define CONS_READ_LINE 2000 //?
#define WITH_SEM 200 //?

#define NONE 0
#define QUIT 1
#define UP 2
#define DOWN 3
#define LEFT 4
#define RIGHT 5

// Prototype des appels systeme de la spec
int chprio(int pid, int newprio);
void cons_write(const char *str, unsigned long size);
#if defined CONS_READ_LINE
unsigned long cons_read(char *string, unsigned long length);
#elif defined CONS_READ_CHAR
int cons_read(void);
#endif
void cons_echo(int on);
void exit(int retval);
int getpid(void);
int getprio(int pid);
int kill(int pid);
#if defined WITH_SEM
int scount(int sem);
int screate(short count);
int sdelete(int sem);
int signal(int sem);
int signaln(int sem, short count);
int sreset(int sem, short count);
int try_wait(int sem);
int wait(int sem);
#elif defined WITH_MSG
int pcount(int fid, int *count);
int pcreate(int count);
int pdelete(int fid);
int preceive(int fid,int *message);
int preset(int fid);
int psend(int fid, int message);
#else
# error "WITH_SEM" ou "WITH_MSG" doit être définie.
#endif

#ifndef TELECOM_TST
void clock_settings(unsigned long *quartz, unsigned long *ticks);
unsigned long current_clock(void);
void wait_clock(unsigned long wakeup);
#endif
int start(const char *process_name, unsigned long ssize, int prio, void *arg);
int waitpid(int pid, int *retval);

#if defined WITH_SEM
/*
 * Pour la soutenance, devrait afficher la liste des processus actifs, des
 * semaphores utilises et toute autre info utile sur le noyau.
 */
#elif defined WITH_MSG
/*
 * Pour la soutenance, devrait afficher la liste des processus actifs, des
 * files de messages utilisees et toute autre info utile sur le noyau.
 */
#endif
void sys_info(void);

/* Shared memory */
void *shm_create(const char*);
void *shm_acquire(const char*);
void shm_release(const char*);


// Additional syscalls

// Colors
typedef enum Color_ {

	// Text & Background colors
	BLACK = 0,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	GRAY,

	// Text only colors
	DARK_GRAY,
	LIGHT_BLUE,
	LIGHT_GREEN,
	LIGHT_CYAN,
	LIGHT_RED,
	LIGHT_MAGENTA,
	YELLOW,
	WHITE,

	NB_COLORS
} Color;

void cons_set_bg_color(Color color);
void cons_set_fg_color(Color color);
void cons_reset_color();

void affiche_etats();
void reboot();
void sleep(unsigned long seconds);
int getppid();
void print_banner();
int set_video_mode();
void get_video_modes(int min_width, int max_width);
void init_vbe_mode(int mode);

//Déplacer le curseur à une position (x,y) de l'écran 
void set_cursor(int lig, int col);

//Afficher un caractère à une position (x,y) de l'écran 
void set_caracter(int lig, int col, char c);

//Renvoyer la largeur de l'écran
int getWidth();

//Renvoyer la hauteur de l'écran
int getHeight();

//Effacer le buffer de l'entrée clavier pour le jeu
void resetInputGame(void);

//Tester l'existence d'une entrée clavier pour le jeu
int testInputGame(void);

//Renvoyer l'entrée clavier pour le jeu la plus récente
int getInputGame(void);

//Attendre pour une entrée clavier
void wait_keyboard(void);

//Ecrire dans le buffer du clavier
void keyboard_data(char *str);
#endif
