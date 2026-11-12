	.file	"vetochka-i64-probe.c"
	.text
	.globl	load_le                         // -- Begin function load_le
	.p2align	2
	.type	load_le,@function
load_le:                                // @load_le
// %bb.0:
	ldr	x0, [x0]
	ret
.Lfunc_end0:
	.size	load_le, .Lfunc_end0-load_le
                                        // -- End function
	.globl	wrap_add                        // -- Begin function wrap_add
	.p2align	2
	.type	wrap_add,@function
wrap_add:                               // @wrap_add
// %bb.0:
	add	x0, x1, x0
	ret
.Lfunc_end1:
	.size	wrap_add, .Lfunc_end1-wrap_add
                                        // -- End function
	.globl	checked_add                     // -- Begin function checked_add
	.p2align	2
	.type	checked_add,@function
checked_add:                            // @checked_add
// %bb.0:
	adds	x8, x0, x1
	cset	w0, vs
	str	x8, [x2]
	ret
.Lfunc_end2:
	.size	checked_add, .Lfunc_end2-checked_add
                                        // -- End function
	.ident	"Ubuntu clang version 20.1.8 (++20250708082409+6fb913d3e2ec-1~exp1~20250708202428.132)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
