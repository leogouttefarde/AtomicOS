	.text
	.globl ctx_sw
# Structure de la pile en entree :
#   %esp + 4 : adresse de l'ancien contexte
#   %esp + 8 : adresse du nouveau contexte
#   %esp + 12 : adresse du pointeur de l'ancien page directory
#   %esp + 16 : pointeur du nouveau page directory
ctx_sw:
	# sauvegarde du contexte de l'ancien processus
	movl 4(%esp), %eax
	movl %ebx, (%eax)
	movl %esp, 4(%eax)
	movl %ebp, 8(%eax)
	movl %esi, 12(%eax)
	movl %edi, 16(%eax)

	# sauvegarde de 4(TSS) comme sommet de la pile kernel (ESP0)
	mov $tss, %ebx
	mov 4(%ebx), %ebx
	movl %ebx, 20(%eax)

	# sauvegarde de cr3 comme ancien pgdir
	movl 12(%esp), %eax
	movl %cr3, %ebx
	movl %ebx, (%eax)


	# restauration du contexte du nouveau processus

	# restauration du nouveau pgdir dans cr3
	movl 16(%esp), %eax
	movl %eax, %cr3

	# stockage de cr3 dans 28(TSS)
	mov $tss, %ebx
	mov %eax, 28(%ebx)

	movl 8(%esp), %eax

	# stockage du sommet de la pile kernel (ESP0) dans 4(TSS)
	movl 20(%eax), %ecx
	movl %ecx, 4(%ebx)

	# SS0
	movl   $0x18, 8(%ebx)

	movl (%eax), %ebx
	movl 4(%eax), %esp
	movl 8(%eax), %ebp
	movl 12(%eax), %esi
	movl 16(%eax), %edi

	# on passe la main au nouveau processus
	ret


	.text
	.globl start_proc
start_proc:
	pushl $0x4b
	# push $0x10
	# pushf
	# push $0x7ffffff8
	push %esp
	pushl $0x202
	pushl $0x43
	# push $0x18
	pushl $0x40000000
	call set_user_protection
	iret

