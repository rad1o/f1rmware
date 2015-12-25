INCLUDE LPC4330_M4_memory.ld

/* Enforce emmition of the vector table. */
EXTERN(vector_table)

/* Define the entry point of the output file. */
ENTRY(reset_handler)

/* Define sections. */
SECTIONS
{
	. = ORIGIN(ram_m0);

	.text : {
		. = ALIGN(0x400);
		*(.vectors)	/* Vector table */
		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
	} >ram_m0

	/* C++ Static constructors/destructors, also used for __attribute__
	 * ((constructor)) and the likes */
	.preinit_array : {
		. = ALIGN(4);
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
	} >ram_m0
	.init_array : {
		. = ALIGN(4);
		__init_array_start = .;
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = .;
	} >ram_m0
	.fini_array : {
		. = ALIGN(4);
		__fini_array_start = .;
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		__fini_array_end = .;
	} >ram_m0

	/*
	 * Another section used by C++ stuff, appears when using newlib with
	 * 64bit (long long) printf support
	 */
	.ARM.extab : {
		*(.ARM.extab*)
	} >ram_m0

	/* exception index - required due to libgcc.a issuing /0 exceptions */
	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
	} >ram_m0

	. = ALIGN(4);
	_etext = .;
	_etext_ram = 0; /* Start of Code in RAM NULL because Copy of Code from ROM to RAM disabled */
	_etext_rom = 0; /* Start of Code in RAM NULL because Copy of Code from ROM to RAM disabled */

	. = ORIGIN(ram_l0dable);
	
	.data : {
		_data = .;
		*(.data*)	/* Read-write initialized data */
		. = ALIGN(4);
	} >ram_m0

	_data_loadaddr = LOADADDR(.data);

	_data = .;
	_edata = .;

	.bss : {
	   _bss = .;
		*(.bss_aligned*)	/* Read-write zero initialized data */
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
	   _ebss = .;
	} >ram_m0

	/* exception unwind data - required due to libgcc.a issuing /0 exceptions */
	.ARM.extab : {
		*(.ARM.extab*)
	} >ram_m0

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }

	. = ALIGN(4);
	end = .;

	/* Leave room above stack for IAP to run. */
	__StackTop = ORIGIN(ram_m0) + LENGTH(ram_m0) - 32;
	PROVIDE(_stack = __StackTop);
}
