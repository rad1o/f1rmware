/* Generic linker script for LPC43XX targets using libopencm3. */

/* Memory regions must be defined in the ld script which includes this one. */

/* Define the entry point of the output file. */
ENTRY(ram)

/* Define sections. */
SECTIONS
{
        .hack (NOLOAD) : { /* hack to advance ">ram_local2" to the current offset */
		KEEP(*(.jump))
                _jumpsize = .;
        } >shadow

	.myhdr : {
		LONG(_jumpsize);
	} >shadow

	.text : {
		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
	} >ram_l0dable AT >shadow

	.ARM.extab : {
		*(.ARM.extab*)
	} >ram_l0dable AT >shadow

	/* exception index - required due to libgcc.a issuing /0 exceptions */
	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
	} >ram_l0dable AT >shadow

	.data : {
		_data = .;
		*(.data*)	/* Read-write initialized data */
		. = ALIGN(4);
		_edata = .;
	} >ram_l0dable AT >shadow

	.bss : {
		_bss = .;
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >ram_l0dable AT >shadow

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }

	. = ALIGN(4);
	_end = .;
}
