
#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__


// Mandatory syscalls
#define START 0
#define GETPID 1
#define GETPRIO 2
#define CHPRIO 3
#define KILL 4
#define WAITPID 5
#define EXIT 6
#define CONS_WRITE 7
#define CONS_READ 8
#define CONS_ECHO 9
#define SCOUNT 10
#define SCREATE 11
#define SDELETE 12
#define SIGNAL 13
#define SIGNALN 14
#define SRESET 15
#define TRY_WAIT 16
#define WAIT 17
#define PCOUNT 18
#define PCREATE 19
#define PDELETE 20
#define PRECEIVE 21
#define PRESET 22
#define PSEND 23
#define CLOCK_SETTINGS 24
#define CURRENT_CLOCK 25
#define WAIT_CLOCK 26
#define SYS_INFO 27
#define SHM_CREATE 28
#define SHM_ACQUIRE 29
#define SHM_RELEASE 30

// Custom syscalls
#define AFFICHE_ETATS 31
#define CONS_RESET_COLOR 32
#define CONS_SET_FG_COLOR 33
#define CONS_SET_BG_COLOR 34
#define REBOOT 35
#define SLEEP 36
#define GETPPID 37
#define PRINT_BANNER 38
#define SET_VIDEO_MODE 39
#define GET_VIDEO_MODES 40
#define SET_CURSOR 41
#define SET_CARACTER 42
#define GETWIDTH 43
#define GETHEIGHT 44
#define RESET_INPUTGAME 45
#define TEST_INPUTGAME 46
#define GET_INPUTGAME 47
#define WAIT_KEYBOARD 48
#define INIT_VBE_MODE 49
#define KEYBOARD_DATA 50
#define CLEAR_LINE 51
#define ATOMICOPEN 52
#define ATOMICREAD 53
#define ATOMICWRITE 54
#define ATOMICCLOSE 55
#define ATOMICEOF 56
#define DISPLAY 57
#define ATOMICLIST 58
#define ATOMICEXISTS 59
#define SET_CONSOLE_MODE 60

#endif
