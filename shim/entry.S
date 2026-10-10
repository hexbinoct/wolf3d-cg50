/*
** Entry points of the shim: the add-in's start, the fx-9860 syscall gate, and the hooks that
** replace the game's hardware routines.
**
** The game was built with Casio's fx-9860 SDK (Renesas SHC). Its calling convention keeps
** r8-r14, PR, MACH and MACL across calls; gcc treats MACH/MACL as scratch, so every way in from
** game code saves them here before calling C.
*/

	.section .text.start, "ax"

/* int shim_start(int isAppli, unsigned short optionNum): the CG50 OS calls this (through the
   jump the converter writes at 0x00300000) the way it calls any add-in's main. */
	.global	_shim_start
	.align	2
_shim_start:
	mov.l	r8, @-r15
	mov.l	r9, @-r15
	mov.l	r10, @-r15
	mov.l	r11, @-r15
	mov.l	r12, @-r15
	mov.l	r13, @-r15
	mov.l	r14, @-r15
	sts.l	pr, @-r15
	sts.l	mach, @-r15
	sts.l	macl, @-r15
	stc.l	gbr, @-r15
	/* shim_init sets up the shim's .data and .bss: only then can the stack be recorded */
	mov	r4, r8
	mov	r5, r9
	mov.l	.Lshim_init, r1
	jsr	@r1
	nop
	mov.l	.Lsaved_sp, r1
	mov.l	r15, @r1
	mov	r8, r4
	mov	r9, r5
	mov.l	.Lshim_main, r1
	jsr	@r1
	nop
	/* shim_main returned: r0 is the add-in's result */

/* Back to the OS from anywhere in the game: the stack is reset to shim_start's frame. */
	.global	_shim_return
_shim_return:
	mov.l	.Lsaved_sp, r1
	mov.l	@r1, r15
	ldc.l	@r15+, gbr
	lds.l	@r15+, macl
	lds.l	@r15+, mach
	lds.l	@r15+, pr
	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r12
	mov.l	@r15+, r11
	mov.l	@r15+, r10
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8

	.align	2
.Lsaved_sp:	.long	_shim_saved_sp
.Lshim_init:	.long	_shim_init
.Lshim_main:	.long	_shim_main

	.text

/* The fx-9860 syscall gate. The game's SDK stubs do 'mov.l #0x80010070,r2; mov.l #n,r0;
   jmp @r2'; the converter replaces each 0x80010070 with this address. Arguments stay where the
   stub left them: r4-r7, more on the stack. C sees
   fx_dispatch(r4, r5, r6, r7, n, caller's stack arguments, return address). */
	.global	_fx_syscall
	.align	2
_fx_syscall:
	mov	r15, r1
	sts.l	pr, @-r15
	sts.l	mach, @-r15
	sts.l	macl, @-r15
	sts	pr, r2
	mov.l	r2, @-r15
	mov.l	r1, @-r15
	mov.l	r0, @-r15
	mov.l	.Lfx_dispatch, r1
	jsr	@r1
	nop
	add	#12, r15
	lds.l	@r15+, macl
	lds.l	@r15+, mach
	lds.l	@r15+, pr
	rts
	nop

	.align	2
.Lfx_dispatch:	.long	_fx_dispatch

/* A hook: the converter overwrites the start of a game routine with a jump here. Arguments are
   in r4-r7 as the game passed them. */
.macro HOOK name, target
	.global	\name
	.align	2
\name:
	sts.l	pr, @-r15
	sts.l	mach, @-r15
	sts.l	macl, @-r15
	mov.l	1f, r1
	jsr	@r1
	nop
	lds.l	@r15+, macl
	lds.l	@r15+, mach
	lds.l	@r15+, pr
	rts
	nop
	.align	2
1:	.long	\target
.endm

	HOOK	_hook_lcd_push, _shim_lcd_push
	HOOK	_hook_lcd_contrast, _shim_lcd_contrast
	HOOK	_hook_rtc_seconds, _shim_rtc_seconds

/* The game's "reset" (jmp 0xA0000000), which it uses to quit: back to the OS menu instead. */
	.global	_hook_exit
	.align	2
_hook_exit:
	mov.l	.Lshim_exit, r1
	jsr	@r1
	nop
	mov.l	.Lshim_return, r1
	jmp	@r1
	mov	#0, r0

	.align	2
.Lshim_exit:	.long	_shim_exit
.Lshim_return:	.long	_shim_return
