
#include <syscalls.h>

	.text
	.globl cons_write
cons_write:
	movl %esp, %ebp
	pushl %eax
	pushl %ebx
	pushl %ecx
	movl CONS_WRITE, %eax
	movl 4(%ebp), %ebx
	movl 8(%ebp), %ecx
	int $49
	popl %ecx
	popl %ebx
	popl %eax
	ret

	.text
	.globl start
start:
	movl $0x22, %eax
	int $7
	ret

	.text
	.globl getpid
getpid:
	movl $0x23, %eax
	int $7
	ret

	.text
	.globl getprio
getprio:
	movl $0x24, %eax
	int $7
	ret

	.text
	.globl chprio
chprio:
	movl $0x25, %eax
	int $7
	ret

	.text
	.globl kill
kill:
	movl $0x26, %eax
	int $7
	ret

	.text
	.globl waitpid
waitpid:
	movl $0x27, %eax
	int $7
	ret

	.text
	.globl exit
exit:
	movl $0x28, %eax
	int $7
	ret

	.text
	.globl cons_read
cons_read:
	movl $0x29, %eax
	int $7
	ret

	.text
	.globl cons_echo
cons_echo:
	movl $0x30, %eax
	int $7
	ret

	.text
	.globl scount
scount:
	movl $0x31, %eax
	int $7
	ret

	.text
	.globl screate
screate:
	movl $0x32, %eax
	int $7
	ret

	.text
	.globl sdelete
sdelete:
	movl $0x33, %eax
	int $7
	ret

	.text
	.globl signal
signal:
	movl $0x34, %eax
	int $7
	ret

	.text
	.globl signaln
signaln:
	movl $0x35, %eax
	int $7
	ret

	.text
	.globl sreset
sreset:
	movl $0x36, %eax
	int $7
	ret

	.text
	.globl try_wait
try_wait:
	movl $0x37, %eax
	int $7
	ret

	.text
	.globl wait
wait:
	movl $0x38, %eax
	int $7
	ret

	.text
	.globl pcount
pcount:
	movl $0x39, %eax
	int $7
	ret

	.text
	.globl pcreate
pcreate:
	movl $0x40, %eax
	int $7
	ret

	.text
	.globl pdelete
pdelete:
	movl $0x41, %eax
	int $7
	ret

	.text
	.globl preceive
preceive:
	movl $0x42, %eax
	int $7
	ret

	.text
	.globl preset
preset:
	movl $0x43, %eax
	int $7
	ret

	.text
	.globl psend
psend:
	movl $0x44, %eax
	int $7
	ret

	.text
	.globl clock_settings
clock_settings:
	movl $0x45, %eax
	int $7
	ret

	.text
	.globl current_clock
current_clock:
	movl $0x46, %eax
	int $7
	ret

	.text
	.globl wait_clock
wait_clock:
	movl $0x47, %eax
	int $7
	ret

	.text
	.globl sys_info
sys_info:
	movl $0x48, %eax
	int $7
	ret

	.text
	.globl shm_create
shm_create:
	movl $0x49, %eax
	int $7
	ret

	.text
	.globl shm_acquire
shm_acquire:
	movl $0x50, %eax
	int $7
	ret

	.text
	.globl shm_release
shm_release:
	movl $0x51, %eax
	int $7
	ret

