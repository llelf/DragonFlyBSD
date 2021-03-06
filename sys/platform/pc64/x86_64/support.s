/*-
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 2003 Peter Wemm.
 * Copyright (c) 2008 The DragonFly Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/amd64/amd64/support.S,v 1.127 2007/05/23 08:33:04 kib Exp $
 */

#include <machine/asmacros.h>
#include <machine/pmap.h>

#include "assym.s"

	ALIGN_DATA

	.text

/*
 * bcopy family
 * void bzero(void *buf, size_t len)
 */

/* done */
ENTRY(bzero)
	movq	%rsi,%rcx
	xorl	%eax,%eax
	shrq	$3,%rcx
	cld
	rep
	stosq
	movq	%rsi,%rcx
	andq	$7,%rcx
	rep
	stosb
	ret

/* Address: %rdi */
ENTRY(pagezero)
	movq	$-PAGE_SIZE,%rdx
	subq	%rdx,%rdi
	xorl	%eax,%eax
1:
	movq	%rax,(%rdi,%rdx)	/* movnti */
	movq	%rax,8(%rdi,%rdx)	/* movnti */
	movq	%rax,16(%rdi,%rdx)	/* movnti */
	movq	%rax,24(%rdi,%rdx)	/* movnti */
	addq	$32,%rdx
	jne	1b
	/*sfence*/
	ret

ENTRY(bcmp)
	movq	%rdx,%rcx
	shrq	$3,%rcx
	cld					/* compare forwards */
	repe
	cmpsq
	jne	1f

	movq	%rdx,%rcx
	andq	$7,%rcx
	repe
	cmpsb
1:
	setne	%al
	movsbl	%al,%eax
	ret

/*
 * bcopy(src, dst, cnt)
 *       rdi, rsi, rdx
 *  ws@tools.de     (Wolfgang Solfrank, TooLs GmbH) +49-228-985800
 */
ENTRY(generic_bcopy)	/* generic_bcopy is bcopy without FPU */
ENTRY(ovbcopy) /* our bcopy doesn't use the FPU, so ovbcopy is the same */
ENTRY(bcopy)
	xchgq	%rsi,%rdi
	movq	%rdx,%rcx

	movq	%rdi,%rax
	subq	%rsi,%rax
	cmpq	%rcx,%rax			/* overlapping && src < dst? */
	jb	1f

	shrq	$3,%rcx				/* copy by 64-bit words */
	cld					/* nope, copy forwards */
	rep
	movsq
	movq	%rdx,%rcx
	andq	$7,%rcx				/* any bytes left? */
	rep
	movsb
	ret

	/* ALIGN_TEXT */
1:
	addq	%rcx,%rdi			/* copy backwards */
	addq	%rcx,%rsi
	decq	%rdi
	decq	%rsi
	andq	$7,%rcx				/* any fractional bytes? */
	std
	rep
	movsb
	movq	%rdx,%rcx			/* copy remainder by 32-bit words */
	shrq	$3,%rcx
	subq	$7,%rsi
	subq	$7,%rdi
	rep
	movsq
	cld
	ret
ENTRY(reset_dbregs)
	movq	$0x200,%rax   /* the manual says that bit 10 must be set to 1 */
	movq    %rax,%dr7     /* disable all breapoints first */
	movq    $0,%rax
	movq    %rax,%dr0
	movq    %rax,%dr1
	movq    %rax,%dr2
	movq    %rax,%dr3
	movq    %rax,%dr6
	ret

/*
 * Note: memcpy does not support overlapping copies
 */
ENTRY(memcpy)
	movq	%rdx,%rcx
	shrq	$3,%rcx				/* copy by 64-bit words */
	cld					/* copy forwards */
	rep
	movsq
	movq	%rdx,%rcx
	andq	$7,%rcx				/* any bytes left? */
	rep
	movsb
	ret

/*
 * pagecopy(%rdi=from, %rsi=to)
 */
ENTRY(pagecopy)
	movq	$-PAGE_SIZE,%rax
	movq	%rax,%rdx
	subq	%rax,%rdi
	subq	%rax,%rsi
1:
	/*prefetchnta (%rdi,%rax)*/
	/*addq	$64,%rax*/
	/*jne	1b*/
2:
	movq	(%rdi,%rdx),%rax
	movq	%rax,(%rsi,%rdx)	/* movnti */
	movq	8(%rdi,%rdx),%rax
	movq	%rax,8(%rsi,%rdx)	/* movnti */
	movq	16(%rdi,%rdx),%rax
	movq	%rax,16(%rsi,%rdx)	/* movnti */
	movq	24(%rdi,%rdx),%rax
	movq	%rax,24(%rsi,%rdx)	/* movnti */
	addq	$32,%rdx
	jne	2b
	/*sfence*/
	ret

/* fillw(pat, base, cnt) */  
/*       %rdi,%rsi, %rdx */
ENTRY(fillw)
	movq	%rdi,%rax   
	movq	%rsi,%rdi
	movq	%rdx,%rcx
	cld
	rep
	stosw
	ret

/*****************************************************************************/
/* copyout and fubyte family                                                 */
/*****************************************************************************/
/*
 * Access user memory from inside the kernel. These routines should be
 * the only places that do this.
 *
 * These routines set curpcb->onfault for the time they execute. When a
 * protection violation occurs inside the functions, the trap handler
 * returns to *curpcb->onfault instead of the function.
 */

/*
 * std_copyout(from_kernel, to_user, len)  - MP SAFE
 *         %rdi,        %rsi,    %rdx
 */
ENTRY(std_copyout)
	movq	PCPU(curthread),%rax
	movq	TD_PCB(%rax), %rax
	movq	$copyout_fault,PCB_ONFAULT(%rax)
	movq	%rsp,PCB_ONFAULT_SP(%rax)
	testq	%rdx,%rdx			/* anything to do? */
	jz	done_copyout

	/*
	 * Check explicitly for non-user addresses.  If 486 write protection
	 * is being used, this check is essential because we are in kernel
	 * mode so the h/w does not provide any protection against writing
	 * kernel addresses.
	 */

	/*
	 * First, prevent address wrapping.
	 */
	movq	%rsi,%rax
	addq	%rdx,%rax
	jc	copyout_fault
/*
 * XXX STOP USING VM_MAX_USER_ADDRESS.
 * It is an end address, not a max, so every time it is used correctly it
 * looks like there is an off by one error, and of course it caused an off
 * by one error in several places.
 */
	movq	$VM_MAX_USER_ADDRESS,%rcx
	cmpq	%rcx,%rax
	ja	copyout_fault

	xchgq	%rdi,%rsi
	/* bcopy(%rsi, %rdi, %rdx) */
	movq	%rdx,%rcx

	shrq	$3,%rcx
	cld
	rep
	movsq
	movb	%dl,%cl
	andb	$7,%cl
	rep
	movsb

done_copyout:
	xorl	%eax,%eax
	movq	PCPU(curthread),%rdx
	movq	TD_PCB(%rdx), %rdx
	movq	%rax,PCB_ONFAULT(%rdx)
	ret

	ALIGN_TEXT
copyout_fault:
	movq	PCPU(curthread),%rdx
	movq	TD_PCB(%rdx), %rdx
	movq	$0,PCB_ONFAULT(%rdx)
	movq	$EFAULT,%rax
	ret

/*
 * std_copyin(from_user, to_kernel, len) - MP SAFE
 *        %rdi,      %rsi,      %rdx
 */
ENTRY(std_copyin)
	movq	PCPU(curthread),%rax
	movq	TD_PCB(%rax), %rax
	movq	$copyin_fault,PCB_ONFAULT(%rax)
	movq	%rsp,PCB_ONFAULT_SP(%rax)
	testq	%rdx,%rdx			/* anything to do? */
	jz	done_copyin

	/*
	 * make sure address is valid
	 */
	movq	%rdi,%rax
	addq	%rdx,%rax
	jc	copyin_fault
	movq	$VM_MAX_USER_ADDRESS,%rcx
	cmpq	%rcx,%rax
	ja	copyin_fault

	xchgq	%rdi,%rsi
	movq	%rdx,%rcx
	movb	%cl,%al
	shrq	$3,%rcx				/* copy longword-wise */
	cld
	rep
	movsq
	movb	%al,%cl
	andb	$7,%cl				/* copy remaining bytes */
	rep
	movsb

done_copyin:
	xorl	%eax,%eax
	movq	PCPU(curthread),%rdx
	movq	TD_PCB(%rdx), %rdx
	movq	%rax,PCB_ONFAULT(%rdx)
	ret

	ALIGN_TEXT
copyin_fault:
	movq	PCPU(curthread),%rdx
	movq	TD_PCB(%rdx), %rdx
	movq	$0,PCB_ONFAULT(%rdx)
	movq	$EFAULT,%rax
	ret

/*
 * casuword32.  Compare and set user integer.  Returns -1 or the current value.
 *        dst = %rdi, old = %rsi, new = %rdx
 */
ENTRY(casuword32)
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)
	movq	%rsp,PCB_ONFAULT_SP(%rcx)

	movq	$VM_MAX_USER_ADDRESS-4,%rax
	cmpq	%rax,%rdi			/* verify address is valid */
	ja	fusufault

	movl	%esi,%eax			/* old */
	lock
	cmpxchgl %edx,(%rdi)			/* new = %edx */

	/*
	 * The old value is in %eax.  If the store succeeded it will be the
	 * value we expected (old) from before the store, otherwise it will
	 * be the current value.
	 */

	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$0,PCB_ONFAULT(%rcx)
	ret

/*
 * casuword.  Compare and set user word.  Returns -1 or the current value.
 *        dst = %rdi, old = %rsi, new = %rdx
 */
ENTRY(casuword)
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)
	movq	%rsp,PCB_ONFAULT_SP(%rcx)

	movq	$VM_MAX_USER_ADDRESS-8,%rax
	cmpq	%rax,%rdi			/* verify address is valid */
	ja	fusufault

	movq	%rsi,%rax			/* old */
	lock
	cmpxchgq %rdx,(%rdi)			/* new = %rdx */

	/*
	 * The old value is in %rax.  If the store succeeded it will be the
	 * value we expected (old) from before the store, otherwise it will
	 * be the current value.
	 */

	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$0,PCB_ONFAULT(%rcx)
	ret

/*
 * Fetch (load) a 64-bit word, a 32-bit word, a 16-bit word, or an 8-bit
 * byte from user memory.  All these functions are MPSAFE.
 * addr = %rdi
 */

ALTENTRY(fuword64)
ENTRY(std_fuword)
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)
	movq	%rsp,PCB_ONFAULT_SP(%rcx)

	movq	$VM_MAX_USER_ADDRESS-8,%rax
	cmpq	%rax,%rdi			/* verify address is valid */
	ja	fusufault

	movq	(%rdi),%rax
	movq	$0,PCB_ONFAULT(%rcx)
	ret

ENTRY(fuword32)
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)
	movq	%rsp,PCB_ONFAULT_SP(%rcx)

	movq	$VM_MAX_USER_ADDRESS-4,%rax
	cmpq	%rax,%rdi			/* verify address is valid */
	ja	fusufault

	movl	(%rdi),%eax
	movq	$0,PCB_ONFAULT(%rcx)
	ret

/*
 * fuswintr() and suswintr() are specialized variants of fuword16() and
 * suword16(), respectively.  They are called from the profiling code,
 * potentially at interrupt time.  If they fail, that's okay; good things
 * will happen later.  They always fail for now, until the trap code is
 * able to deal with this.
 */
ALTENTRY(suswintr)
ENTRY(fuswintr)
	movq	$-1,%rax
	ret

ENTRY(fuword16)
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)
	movq	%rsp,PCB_ONFAULT_SP(%rcx)

	movq	$VM_MAX_USER_ADDRESS-2,%rax
	cmpq	%rax,%rdi
	ja	fusufault

	movzwl	(%rdi),%eax
	movq	$0,PCB_ONFAULT(%rcx)
	ret

ENTRY(std_fubyte)
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)
	movq	%rsp,PCB_ONFAULT_SP(%rcx)

	movq	$VM_MAX_USER_ADDRESS-1,%rax
	cmpq	%rax,%rdi
	ja	fusufault

	movzbl	(%rdi),%eax
	movq	$0,PCB_ONFAULT(%rcx)
	ret

	ALIGN_TEXT
fusufault:
	movq	PCPU(curthread),%rcx
	xorl	%eax,%eax
	movq	TD_PCB(%rcx), %rcx
	movq	%rax,PCB_ONFAULT(%rcx)
	decq	%rax
	ret

/*
 * Store a 64-bit word, a 32-bit word, a 16-bit word, or an 8-bit byte to
 * user memory.  All these functions are MPSAFE.
 *
 * addr = %rdi, value = %rsi
 *
 * Write a long
 */
ALTENTRY(suword64)
ENTRY(std_suword)
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)
	movq	%rsp,PCB_ONFAULT_SP(%rcx)

	movq	$VM_MAX_USER_ADDRESS-8,%rax
	cmpq	%rax,%rdi			/* verify address validity */
	ja	fusufault

	movq	%rsi,(%rdi)
	xorl	%eax,%eax
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	%rax,PCB_ONFAULT(%rcx)
	ret

/*
 * Write an int
 */
ENTRY(std_suword32)
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)
	movq	%rsp,PCB_ONFAULT_SP(%rcx)

	movq	$VM_MAX_USER_ADDRESS-4,%rax
	cmpq	%rax,%rdi			/* verify address validity */
	ja	fusufault

	movl	%esi,(%rdi)
	xorl	%eax,%eax
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	%rax,PCB_ONFAULT(%rcx)
	ret

ENTRY(suword16)
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)
	movq	%rsp,PCB_ONFAULT_SP(%rcx)

	movq	$VM_MAX_USER_ADDRESS-2,%rax
	cmpq	%rax,%rdi			/* verify address validity */
	ja	fusufault

	movw	%si,(%rdi)
	xorl	%eax,%eax
	movq	PCPU(curthread),%rcx		/* restore trashed register */
	movq	TD_PCB(%rcx), %rcx
	movq	%rax,PCB_ONFAULT(%rcx)
	ret

ENTRY(std_subyte)
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$fusufault,PCB_ONFAULT(%rcx)
	movq	%rsp,PCB_ONFAULT_SP(%rcx)

	movq	$VM_MAX_USER_ADDRESS-1,%rax
	cmpq	%rax,%rdi			/* verify address validity */
	ja	fusufault

	movl	%esi,%eax
	movb	%al,(%rdi)
	xorl	%eax,%eax
	movq	PCPU(curthread),%rcx		/* restore trashed register */
	movq	TD_PCB(%rcx), %rcx
	movq	%rax,PCB_ONFAULT(%rcx)
	ret

/*
 * std_copyinstr(from, to, maxlen, int *lencopied) - MP SAFE
 *           %rdi, %rsi, %rdx, %rcx
 *
 *	copy a string from from to to, stop when a 0 character is reached.
 *	return ENAMETOOLONG if string is longer than maxlen, and
 *	EFAULT on protection violations. If lencopied is non-zero,
 *	return the actual length in *lencopied.
 */
ENTRY(std_copyinstr)
	movq	%rdx,%r8			/* %r8 = maxlen */
	movq	%rcx,%r9			/* %r9 = *len */
	xchgq	%rdi,%rsi			/* %rdi = from, %rsi = to */
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$cpystrflt,PCB_ONFAULT(%rcx)
	movq	%rsp,PCB_ONFAULT_SP(%rcx)

	movq	$VM_MAX_USER_ADDRESS,%rax

	/* make sure 'from' is within bounds */
	subq	%rsi,%rax
	jbe	cpystrflt

	/* restrict maxlen to <= VM_MAX_USER_ADDRESS-from */
	cmpq	%rdx,%rax
	jae	1f
	movq	%rax,%rdx
	movq	%rax,%r8
1:
	incq	%rdx
	cld

2:
	decq	%rdx
	jz	3f

	lodsb
	stosb
	orb	%al,%al
	jnz	2b

	/* Success -- 0 byte reached */
	decq	%rdx
	xorl	%eax,%eax
	jmp	cpystrflt_x
3:
	/* rdx is zero - return ENAMETOOLONG or EFAULT */
	movq	$VM_MAX_USER_ADDRESS,%rax
	cmpq	%rax,%rsi
	jae	cpystrflt
4:
	movq	$ENAMETOOLONG,%rax
	jmp	cpystrflt_x

cpystrflt:
	movq	$EFAULT,%rax

cpystrflt_x:
	/* set *lencopied and return %eax */
	movq	PCPU(curthread),%rcx
	movq	TD_PCB(%rcx), %rcx
	movq	$0,PCB_ONFAULT(%rcx)

	testq	%r9,%r9
	jz	1f
	subq	%rdx,%r8
	movq	%r8,(%r9)
1:
	ret


/*
 * copystr(from, to, maxlen, int *lencopied) - MP SAFE
 *         %rdi, %rsi, %rdx, %rcx
 */
ENTRY(copystr)
	movq	%rdx,%r8			/* %r8 = maxlen */

	xchgq	%rdi,%rsi
	incq	%rdx
	cld
1:
	decq	%rdx
	jz	4f
	lodsb
	stosb
	orb	%al,%al
	jnz	1b

	/* Success -- 0 byte reached */
	decq	%rdx
	xorl	%eax,%eax
	jmp	6f
4:
	/* rdx is zero -- return ENAMETOOLONG */
	movq	$ENAMETOOLONG,%rax

6:

	testq	%rcx,%rcx
	jz	7f
	/* set *lencopied and return %rax */
	subq	%rdx,%r8
	movq	%r8,(%rcx)
7:
	ret

/*
 * Handling of special x86_64 registers and descriptor tables etc
 * %rdi
 */
/* void lgdt(struct region_descriptor *rdp); */
ENTRY(lgdt)
	/* reload the descriptor table */
	lgdt	(%rdi)

	/* flush the prefetch q */
	jmp	1f
	nop
1:
	movl	$KDSEL,%eax
	movl	%eax,%ds
	movl	%eax,%es
	movl	%eax,%fs	/* Beware, use wrmsr to set 64 bit base */
	movl	%eax,%gs	/* Beware, use wrmsr to set 64 bit base */
	movl	%eax,%ss

	/* reload code selector by turning return into intersegmental return */
	popq	%rax
	pushq	$KCSEL
	pushq	%rax
	MEXITCOUNT
	lretq

/*****************************************************************************/
/* setjmp, longjmp                                                           */
/*****************************************************************************/

ENTRY(setjmp)
	movq	%rbx,0(%rdi)			/* save rbx */
	movq	%rsp,8(%rdi)			/* save rsp */
	movq	%rbp,16(%rdi)			/* save rbp */
	movq	%r12,24(%rdi)			/* save r12 */
	movq	%r13,32(%rdi)			/* save r13 */
	movq	%r14,40(%rdi)			/* save r14 */
	movq	%r15,48(%rdi)			/* save r15 */
	movq	0(%rsp),%rdx			/* get rta */
	movq	%rdx,56(%rdi)			/* save rip */
	xorl	%eax,%eax			/* return(0); */
	ret

ENTRY(longjmp)
	movq	0(%rdi),%rbx			/* restore rbx */
	movq	8(%rdi),%rsp			/* restore rsp */
	movq	16(%rdi),%rbp			/* restore rbp */
	movq	24(%rdi),%r12			/* restore r12 */
	movq	32(%rdi),%r13			/* restore r13 */
	movq	40(%rdi),%r14			/* restore r14 */
	movq	48(%rdi),%r15			/* restore r15 */
	movq	56(%rdi),%rdx			/* get rta */
	movq	%rdx,0(%rsp)			/* put in return frame */
	xorl	%eax,%eax			/* return(1); */
	incl	%eax
	ret

/*
 * Support for reading MSRs in the safe manner.
 */
ENTRY(rdmsr_safe)
/* int rdmsr_safe(u_int msr, uint64_t *data) */
	movq	PCPU(curthread),%r8
	movq	TD_PCB(%r8), %r8
	movq	$msr_onfault,PCB_ONFAULT(%r8)
	movq	%rsp,PCB_ONFAULT_SP(%r8)
	movl	%edi,%ecx
	rdmsr			/* Read MSR pointed by %ecx. Returns
				   hi byte in edx, lo in %eax */
	salq	$32,%rdx	/* sign-shift %rdx left */
	movl	%eax,%eax	/* zero-extend %eax -> %rax */
	orq	%rdx,%rax
	movq	%rax,(%rsi)
	xorq	%rax,%rax
	movq	%rax,PCB_ONFAULT(%r8)
	ret

/*
 * Support for writing MSRs in the safe manner.
 */
ENTRY(wrmsr_safe)
/* int wrmsr_safe(u_int msr, uint64_t data) */
	movq	PCPU(curthread),%r8
	movq	TD_PCB(%r8), %r8
	movq	$msr_onfault,PCB_ONFAULT(%r8)
	movq    %rsp,PCB_ONFAULT_SP(%rcx)
	movl	%edi,%ecx
	movl	%esi,%eax
	sarq	$32,%rsi
	movl	%esi,%edx
	wrmsr			/* Write MSR pointed by %ecx. Accepts
				   hi byte in edx, lo in %eax. */
	xorq	%rax,%rax
	movq	%rax,PCB_ONFAULT(%r8)
	ret

/*
 * MSR operations fault handler
 */
	ALIGN_TEXT
msr_onfault:
	movq	PCPU(curthread),%r8
	movq	TD_PCB(%r8), %r8
	movq	$0,PCB_ONFAULT(%r8)
	movl	$EFAULT,%eax
	ret

/*
 * Support for BB-profiling (gcc -a).  The kernbb program will extract
 * the data from the kernel.
 */

	.data
	ALIGN_DATA
	.globl bbhead
bbhead:
	.quad 0

	.text
NON_GPROF_ENTRY(__bb_init_func)
	movq	$1,(%rdi)
	movq	bbhead,%rax
	movq	%rax,32(%rdi)
	movq	%rdi,bbhead
	NON_GPROF_RET
