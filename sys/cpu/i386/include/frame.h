/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *	from: @(#)frame.h	5.2 (Berkeley) 1/18/91
 * $FreeBSD: src/sys/i386/include/frame.h,v 1.20 1999/09/29 15:06:22 marcel Exp $
 * $DragonFly: src/sys/cpu/i386/include/frame.h,v 1.9 2007/01/23 08:43:02 dillon Exp $
 */

#ifndef _CPU_FRAME_H_
#define _CPU_FRAME_H_

/*
 * System stack frames.
 */

/*
 * Exception/Trap Stack Frame.  This frame must match or be embedded within
 * all other frame types, including signal context frames.
 */
struct trapframe {
	int	tf_gs;
	int	tf_fs;
	int	tf_es;
	int	tf_ds;
	int	tf_edi;
	int	tf_esi;
	int	tf_ebp;
	int	tf_isp;
	int	tf_ebx;
	int	tf_edx;
	int	tf_ecx;
	int	tf_eax;
	int	tf_xflags;
	int	tf_trapno;
	/* below portion defined in 386 hardware */
	int	tf_err;
	int	tf_eip;
	int	tf_cs;
	int	tf_eflags;
	/* below only when crossing rings (e.g. user to kernel) */
#define tf_sp tf_esp
	int	tf_esp;
	int	tf_ss;
};

/*
 * This frame is postfixed with additinoal information for traps from
 * virtual-8086 mode but must otherwise match the trapframe.
 */
struct trapframe_vm86 {
	int	tf_gs;
	int	tf_fs;
	int	tf_es;
	int	tf_ds;
	int	tf_edi;
	int	tf_esi;
	int	tf_ebp;
	int	tf_isp;
	int	tf_ebx;
	int	tf_edx;
	int	tf_ecx;
	int	tf_eax;
	int	tf_xflags;
	int	tf_trapno;
	/* below portion defined in 386 hardware */
	int	tf_err;
	int	tf_eip;
	int	tf_cs;
	int	tf_eflags;
	/* below only when crossing rings (e.g. user to kernel) */
	int	tf_esp;
	int	tf_ss;
	/* below only when switching out of VM86 mode */
	int	tf_vm86_es;
	int	tf_vm86_ds;
	int	tf_vm86_fs;
	int	tf_vm86_gs;
};

/*
 * Interrupt stack frame.  This frame is prefixed with additional
 * information but must otherwise match the trapframe.
 */
struct intrframe {
	int	if_vec;
	int	if_ppl;
	int	if_gs;
	int	if_fs;
	int	if_es;
	int	if_ds;
	int	if_edi;
	int	if_esi;
	int	if_ebp;
	int	if_isp;		/* unused/trap frame compat - isp */
	int	if_ebx;
	int	if_edx;
	int	if_ecx;
	int	if_eax;
	int	if_xflags;	/* trap frame compat - xflags (vkernel) */
	int	if_trapno;	/* unused/trap frame compat - trapno */
	int	if_err;		/* unused/trap frame compat - err */
	/* below portion defined in 386 hardware */
	int	if_eip;
	int	if_cs;
	int	if_eflags;
	/* below only when crossing rings (e.g. user to kernel) */
	int	if_esp;
	int	if_ss;
};

int	kdb_trap (int, int, struct trapframe *);
extern  int (*pmath_emulate) (struct trapframe *);

#define	INTR_TO_TRAPFRAME(frame) ((struct trapframe *)&(frame)->if_gs)

#endif /* _CPU_FRAME_H_ */
