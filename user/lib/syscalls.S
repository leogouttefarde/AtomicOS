
#include <syscalls.h>

/*
	%eax : numéro d'interruption

	Passage des paramètres dans l'ordre suivant :
	%ebx, %ecx, %edx, %esi, %edi

	Registres à ne pas sauvegarder (scratch) :
	%ecx, %edx, %edi

	Registres à sauvegarder :
	%ebp, %ebx, %esi
*/

	.text
	.globl start
start:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	pushl %esi
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl 16(%ebp), %edx
	movl 20(%ebp), %esi
	movl $START, %eax
	int $49
	popl %esi
	popl %ebx
	popl %ebp
	ret

	.text
	.globl getpid
getpid:
	movl $GETPID, %eax
	int $49
	ret

	.text
	.globl getprio
getprio:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $GETPRIO, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl chprio
chprio:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $CHPRIO, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl kill
kill:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $KILL, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl waitpid
waitpid:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $WAITPID, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl exit
exit:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $EXIT, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl cons_write
cons_write:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $CONS_WRITE, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl cons_read
cons_read:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $CONS_READ, %eax
	int $49
	popl %ebx
	popl %ebp
	ret	

	.text
	.globl cons_echo
cons_echo:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $CONS_ECHO, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl scount
scount:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $SCOUNT, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl screate
screate:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $SCREATE, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl sdelete
sdelete:	
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $SDELETE, %eax
	int $49
	popl %ebx
	popl %ebp
	ret
	
	.text
	.globl signal
signal:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $SIGNAL, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl signaln
signaln:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $SIGNALN, %eax
	int $49
	popl %ebx
	popl %ebp
	ret
	
	.text
	.globl sreset
sreset:	
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $SRESET, %eax
	int $49
	popl %ebx
	popl %ebp
	ret	
	
	.text
	.globl try_wait
try_wait:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $TRY_WAIT, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl wait
wait:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $WAIT, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl pcount
pcount:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $PCOUNT, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl pcreate
pcreate:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $PCREATE, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl pdelete
pdelete:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $PDELETE, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl preceive
preceive:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $PRECEIVE, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl preset
preset:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $PRESET, %eax
	int $49
	popl %ebx
	popl %ebp
	ret
	
	.text
	.globl psend
psend:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $PSEND, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl clock_settings
clock_settings:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $CLOCK_SETTINGS, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl current_clock
current_clock:
	movl $CURRENT_CLOCK, %eax
	int $49
	ret
	
	.text
	.globl wait_clock
wait_clock:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $WAIT_CLOCK, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl sys_info
sys_info:
	movl $SYS_INFO, %eax
	int $49
	ret

	.text
	.globl shm_create
shm_create:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $SHM_CREATE, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl shm_acquire
shm_acquire:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $SHM_ACQUIRE, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl shm_release
shm_release:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $SHM_RELEASE, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl affiche_etats
affiche_etats:
	movl $AFFICHE_ETATS, %eax
	int $49
	ret

	.text
	.globl cons_reset_color
cons_reset_color:
	movl $CONS_RESET_COLOR, %eax
	int $49
	ret

	.text
	.globl cons_set_fg_color
cons_set_fg_color:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $CONS_SET_FG_COLOR, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl cons_set_bg_color
cons_set_bg_color:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $CONS_SET_BG_COLOR, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl reboot
reboot:
	movl $REBOOT, %eax
	int $49
	ret

	.text
	.globl sleep
sleep:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl $SLEEP, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl getppid
getppid:
	movl $GETPPID, %eax
	int $49
	ret

	.text
	.globl print_banner
print_banner:
	movl $PRINT_BANNER, %eax
	int $49
	ret

	.text
	.globl set_video_mode
set_video_mode:
	movl $SET_VIDEO_MODE, %eax
	int $49
	ret

	.text
	.globl get_video_modes
get_video_modes:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $GET_VIDEO_MODES, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl set_cursor
set_cursor:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl $SET_CURSOR, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl set_caracter
set_caracter:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl 16(%ebp), %edx
	movl $SET_CARACTER, %eax
	int $49
	popl %ebx
	popl %ebp
	ret

	.text
	.globl getWidth
getWidth:
	movl $GETWIDTH, %eax
	int $49
	ret

	.text
	.globl getHeight
getHeight:
	movl $GETHEIGHT, %eax
	int $49
	ret

	.text
	.globl resetInputGame
resetInputGame:
	movl $RESET_INPUTGAME, %eax
	int $49
	ret	

	.text
	.globl testInputGame
testInputGame:
	movl $TEST_INPUTGAME, %eax
	int $49
	ret

	.text
	.globl getInputGame
getInputGame:
	movl $GET_INPUTGAME, %eax
	int $49
	ret

	.text
	.globl wait_keyboard
wait_keyboard:
	movl $WAIT_KEYBOARD, %eax
	int $49
	ret

