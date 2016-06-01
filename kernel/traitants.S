	.text
# cette directive sert a rendre l'etiquette publique
	.globl traitant_IT_32
# debut du traitant 
traitant_IT_32:
	call set_kernel_protection
# sauvegarde des registres importants
	pushl %eax
	pushl %edx
	pushl %ecx
# appel a la fonction C realisant le traitant
	call tic_PIT
# restauration des registres importants
	popl %ecx
	popl %edx
	popl %eax
# fin du traitant
	call set_user_protection
	iret

#traitant clavier
	.text
# cette directive sert a rendre l'etiquette publique
	.globl traitant_IT_33
#debut du traitant
traitant_IT_33:
	call set_kernel_protection
# sauvegarde des registres importants
	pushl %eax
	pushl %edx
	pushl %ecx
# appel a la fonction C realisant le traitant
	call traitant_clavier
# restauration des registres importants
	popl %ecx
	popl %edx
	popl %eax
	call set_user_protection
# fin du traitant
	iret

#traitant clavier
	.text
# cette directive sert a rendre l'etiquette publique
	.globl traitant_IT_44
#debut du traitant
traitant_IT_44:
	pusha
	call set_kernel_protection
# sauvegarde des registres importants
	pushl %eax
	pushl %ebx
	pushl %esi
	pushl %edx
	pushl %ecx
# appel a la fonction C realisant le traitant
	call mouse_handler
# restauration des registres importants
	popl %ecx
	popl %edx
	popl %esi
	popl %ebx
	popl %eax
	call set_user_protection
	popa
# fin du traitant
	iret

#traitant d'appels système
	.text
# cette directive sert a rendre l'etiquette publique
	.globl traitant_IT_49
#debut du traitant
traitant_IT_49:
# # sauvegarde des registres importants
# 	pushl %eax
# 	pushl %edx
# 	pushl %ecx
# # appel a la fonction C realisant le traitant
# 	call syscall
# # restauration des registres importants
# 	popl %ecx
# 	popl %edx
# 	popl %eax
# # fin du traitant
# 	iret
	call set_kernel_protection

	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	call syscall
	movl %eax, %ecx
	add $24, %esp

	call set_user_protection
	movl %ecx, %eax
	iret

	.text
	.globl page_fault_IT
page_fault_IT:
	popl %ebx
	call set_kernel_protection
	pushl %eax
	pushl %edx
	pushl %ecx
	pushl %ebx
	movl %cr2, %ebx
	pushl %ebx
	call page_fault_handler
	add $8, %esp
	popl %ecx
	popl %edx
	popl %eax
	call set_user_protection
	iret

	.text
	.globl exception_IT
exception_IT:
	call set_kernel_protection
	pushl %eax
	pushl %edx
	pushl %ecx
	call exception_handler
	popl %ecx
	popl %edx
	popl %eax
	call set_user_protection
	iret

	.text
	.globl exception_IT_pop
exception_IT_pop:
	popl %ebx
	call set_kernel_protection
	pushl %eax
	pushl %edx
	pushl %ecx
	pushl %ebx
	call exception_handler_pop
	add $4, %esp
	popl %ecx
	popl %edx
	popl %eax
	call set_user_protection
	iret

	.text
	.globl set_user_protection
set_user_protection:
	pushl %eax
	movl $0x4b, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	popl %eax
	ret

	.text
	.globl set_kernel_protection
set_kernel_protection:
	pushl %eax
	movl $0x18,%eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	popl %eax
	ret
