/*
** The shim sits after the game in the add-in image: the game's code keeps its fx-9860 addresses
** (0x00300200 on) and its RAM (0x08100000-0x0810206C); the shim's code goes at 0x00320000 and
** its data well above the game's, far below the stack at the top of the add-in RAM.
*/

OUTPUT_ARCH(sh4)
ENTRY(_shim_start)

MEMORY
{
	rom (rx) : o = 0x00320000, l = 256K
	ram (rw) : o = 0x08110000, l = 64K
}

SECTIONS
{
	/* the gate and the hooks are only referenced by the patched game: keep them */
	.text : {
		KEEP(*(.text.start))
		KEEP(*entry.o(.text))
		*(.text .text.*)
	} > rom

	.rodata : {
		*(.rodata .rodata.*)
		. = ALIGN(4);
	} > rom

	.data : ALIGN(4) {
		_sdata = .;
		*(.data .data.*)
		. = ALIGN(4);
		_edata = .;
	} > ram AT> rom
	_ldata = LOADADDR(.data);

	.bss (NOLOAD) : ALIGN(4) {
		_sbss = .;
		*(.bss .bss.* COMMON)
		. = ALIGN(4);
		_ebss = .;
	} > ram

	/DISCARD/ : {
		*(.comment) *(.debug*) *(.note*) *(.eh_frame*)
	}
}
