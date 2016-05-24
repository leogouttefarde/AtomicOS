
#include <syscalls.h>

/*
	%eax : numéro d'interruption

	Passage des paramètres dans l'ordre suivant :
	%ebx, %ecx, %edx, %esi, %edi

	Registres à ne pas sauvegarder (scratch) :
	%ecx, %edx, %esi, %edi

	Registres à sauvegarder :
	%ebp, %ebx
*/

	.text
	.globl start
start:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	movl 8(%ebp), %ebx
	movl 12(%ebp), %ecx
	movl 16(%ebp), %edx
	movl 20(%ebp), %esi
	movl $START, %eax
	int $49
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
	movl $CONS_ECHO, %eax
	int $49
	ret

	.text
	.globl scount
scount:
	movl $0x31, %eax
	int $3
	ret

	.text
	.globl screate
screate:
	movl $0x32, %eax
	int $3
	ret

	.text
	.globl sdelete
sdelete:
	movl $0x33, %eax
	int $3
	ret

	.text
	.globl signal
signal:
	movl $0x34, %eax
	int $3
	ret

	.text
	.globl signaln
signaln:
	movl $0x35, %eax
	int $3
	ret

	.text
	.globl sreset
sreset:
	movl $0x36, %eax
	int $3
	ret

	.text
	.globl try_wait
try_wait:
	movl $0x37, %eax
	int $3
	ret

	.text
	.globl wait
wait:
	movl $0x38, %eax
	int $3
	ret

	.text
	.globl pcount
pcount:
	movl $0x39, %eax
	int $3
	ret

	.text
	.globl pcreate
pcreate:
	movl $0x40, %eax
	int $3
	ret

	.text
	.globl pdelete
pdelete:
	movl $0x41, %eax
	int $3
	ret

	.text
	.globl preceive
preceive:
	movl $0x42, %eax
	int $3
	ret

	.text
	.globl preset
preset:
	movl $0x43, %eax
	int $3
	ret

	.text
	.globl psend
psend:
	movl $0x44, %eax
	int $3
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
	movl $0x48, %eax
	int $3
	ret

	.text
	.globl shm_create
shm_create:
	movl $0x49, %eax
	int $3
	ret

	.text
	.globl shm_acquire
shm_acquire:
	movl $0x50, %eax
	int $3
	ret

	.text
	.globl shm_release
shm_release:
	movl $0x51, %eax
	int $3
	ret

