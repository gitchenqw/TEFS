	.file	"test.cpp"
	.globl	inform
	.bss
	.align 32
	.type	inform, @object
	.size	inform, 4096
inform:
	.zero	4096
	.globl	current
	.align 4
	.type	current, @object
	.size	current, 4
current:
	.zero	4
	.text
	.globl	_Z11GetServerIpv
	.type	_Z11GetServerIpv, @function
_Z11GetServerIpv:
.LFB0:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$16, %esp
	movl	$0, -8(%ebp)
	jmp	.L6
.L7:
	nop
.L6:
	movl	current, %eax
	movl	%eax, %edx
	sarl	$31, %edx
	shrl	$22, %edx
	addl	%edx, %eax
	andl	$1023, %eax
	subl	%edx, %eax
	movl	%eax, -4(%ebp)
	movl	-4(%ebp), %eax
	movl	inform(,%eax,4), %eax
	testl	%eax, %eax
	je	.L2
	movl	-4(%ebp), %eax
	movl	%eax, current
	nop
	movl	current, %eax
	addl	$1, %eax
	movl	%eax, current
	movl	-4(%ebp), %eax
	movl	inform(,%eax,4), %eax
	jmp	.L5
.L2:
	movl	current, %eax
	addl	$1, %eax
	movl	%eax, current
	cmpl	$1024, -8(%ebp)
	setg	%al
	addl	$1, -8(%ebp)
	testb	%al, %al
	je	.L7
	movl	$0, %eax
.L5:
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE0:
	.size	_Z11GetServerIpv, .-_Z11GetServerIpv
	.globl	_Z3funv
	.type	_Z3funv, @function
_Z3funv:
.LFB1:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$16, %esp
	addl	$1, -4(%ebp)
	subl	$1, -4(%ebp)
	movl	$1, %eax
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE1:
	.size	_Z3funv, .-_Z3funv
	.section	.rodata
.LC0:
	.string	"hello"
	.text
	.globl	main
	.type	main, @function
main:
.LFB2:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	andl	$-16, %esp
	subl	$32, %esp
	movl	$9437184, (%esp)
	call	_Znwj
	movl	%eax, 24(%esp)
	movl	$9437184, (%esp)
	call	_Znwj
	movl	%eax, 28(%esp)
	movl	$.LC0, (%esp)
	call	puts
	movl	$0, %eax
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE2:
	.size	main, .-main
	.ident	"GCC: (Ubuntu/Linaro 4.6.3-1ubuntu5) 4.6.3"
	.section	.note.GNU-stack,"",@progbits
