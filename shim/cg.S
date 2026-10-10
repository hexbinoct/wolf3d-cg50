/*
** CG50 OS syscalls (OS 3.60 numbers; the handlers were checked against the 3.60 table with
** casio-cg50/re/syscall360.py).
*/

.macro CGSYS name, number
	.global	\name
	.align	2
\name:
	mov.l	1f, r2
	mov.l	2f, r0
	jmp	@r2
	nop
	.align	2
1:	.long	0x80020070
2:	.long	\number
.endm

	.text
	CGSYS	_cg_GetVRAMAddress,	0x01E6	/* void *(void) */
	CGSYS	_cg_PutDisp_DD,		0x025F	/* void (void): all 216 VRAM rows */
	CGSYS	_cg_PutDisp_DD_stripe,	0x0260	/* void (int y1, int y2) */
	CGSYS	_cg_AllClr_VRAM,	0x0272	/* void (void) */
	CGSYS	_cg_EnableStatusArea,	0x02B7	/* void (int opt): 3 = off */
	CGSYS	_cg_RTC_GetTicks,	0x02C1	/* int (void): 1/128 s */
	CGSYS	_cg_Timer_Install,	0x08D9	/* int (int id, void (*)(void), int ms) */
	CGSYS	_cg_Timer_Deinstall,	0x08DA	/* int (int id) */
	CGSYS	_cg_Timer_Start,	0x08DB	/* int (int id) */
	CGSYS	_cg_Timer_Stop,		0x08DC	/* int (int id) */
	CGSYS	_cg_GetKey,		0x0EAB	/* int (int *key) */
	CGSYS	_cg_WaitMs,		0x1BB4	/* void (int ms) */
	CGSYS	_cg_Bfile_Open,		0x1DA3	/* int (const u16 *name, int mode, int zero) */
	CGSYS	_cg_Bfile_Close,	0x1DA4	/* int (int h) */
	CGSYS	_cg_Bfile_Size,		0x1DA6	/* int (int h) */
	CGSYS	_cg_Bfile_Seek,		0x1DA9	/* int (int h, int pos) */
	CGSYS	_cg_Bfile_Read,		0x1DAC	/* int (int h, void *buf, int size, int readpos) */
	CGSYS	_cg_Bfile_Create,	0x1DAE	/* int (const u16 *name, int mode, int *size) */
	CGSYS	_cg_Bfile_Write,	0x1DAF	/* int (int h, const void *buf, int size) */
	CGSYS	_cg_Bfile_Delete,	0x1DB4	/* int (const u16 *name) */
