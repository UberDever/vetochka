	.file	"vetochka-i64-probe.c"
	.text
	.globl	load_le                         # -- Begin function load_le
	.p2align	4
	.type	load_le,@function
load_le:                                # @load_le
	.cfi_startproc
# %bb.0:
	movq	(%rdi), %rax
	retq
.Lfunc_end0:
	.size	load_le, .Lfunc_end0-load_le
	.cfi_endproc
                                        # -- End function
	.globl	wrap_add                        # -- Begin function wrap_add
	.p2align	4
	.type	wrap_add,@function
wrap_add:                               # @wrap_add
	.cfi_startproc
# %bb.0:
	leaq	(%rdi,%rsi), %rax
	retq
.Lfunc_end1:
	.size	wrap_add, .Lfunc_end1-wrap_add
	.cfi_endproc
                                        # -- End function
	.globl	checked_add                     # -- Begin function checked_add
	.p2align	4
	.type	checked_add,@function
checked_add:                            # @checked_add
	.cfi_startproc
# %bb.0:
	xorl	%eax, %eax
	addq	%rsi, %rdi
	seto	%al
	movq	%rdi, (%rdx)
	retq
.Lfunc_end2:
	.size	checked_add, .Lfunc_end2-checked_add
	.cfi_endproc
                                        # -- End function
	.ident	"Ubuntu clang version 20.1.8 (++20250708082409+6fb913d3e2ec-1~exp1~20250708202428.132)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
