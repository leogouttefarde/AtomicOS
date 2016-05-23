
#include <syscalls.h>

	.globl start
start:
	movl $0x22, %eax
	int $3
	ret

	.globl getpid
getpid:
	movl $0x23, %eax
	int $3
	ret

	.globl getprio
getprio:
	movl $0x24, %eax
	int $3
	ret

	.globl chprio
chprio:
	movl $0x25, %eax
	int $3
	ret

	.globl kill
kill:
	movl $0x26, %eax
	int $3
	ret

	.globl waitpid
waitpid:
	movl $0x27, %eax
	int $3
	ret

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

	.globl cons_read
cons_read:
	movl $0x29, %eax
	int $3
	ret

	.globl cons_echo
cons_echo:
	movl $0x30, %eax
	int $3
	ret

	.globl scount
scount:
	movl $0x31, %eax
	int $3
	ret

	.globl screate
screate:
	movl $0x32, %eax
	int $3
	ret

	.globl sdelete
sdelete:
	movl $0x33, %eax
	int $3
	ret

	.globl signal
signal:
	movl $0x34, %eax
	int $3
	ret

	.globl signaln
signaln:
	movl $0x35, %eax
	int $3
	ret

	.globl sreset
sreset:
	movl $0x36, %eax
	int $3
	ret

	.globl try_wait
try_wait:
	movl $0x37, %eax
	int $3
	ret

	.globl wait
wait:
	movl $0x38, %eax
	int $3
	ret

	.globl pcount
pcount:
	movl $0x39, %eax
	int $3
	ret

	.globl pcreate
pcreate:
	movl $0x40, %eax
	int $3
	ret

	.globl pdelete
pdelete:
	movl $0x41, %eax
	int $3
	ret

	.globl preceive
preceive:
	movl $0x42, %eax
	int $3
	ret

	.globl preset
preset:
	movl $0x43, %eax
	int $3
	ret

	.globl psend
psend:
	movl $0x44, %eax
	int $3
	ret

	.globl clock_settings
clock_settings:
	movl $0x45, %eax
	int $3
	ret

	.globl current_clock
current_clock:
	movl $0x46, %eax
	int $3
	ret

	.globl wait_clock
wait_clock:
	movl $0x47, %eax
	int $3
	ret

	.globl sys_info
sys_info:
	movl $0x48, %eax
	int $3
	ret

	.globl shm_create
shm_create:
	movl $0x49, %eax
	int $3
	ret

	.globl shm_acquire
shm_acquire:
	movl $0x50, %eax
	int $3
	ret

	.globl shm_release
shm_release:
	movl $0x51, %eax
	int $3
	ret

