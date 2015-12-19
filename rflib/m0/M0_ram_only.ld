MEMORY
{
	/* rom is really the shadow region that points to SPI flash or elsewhere */
	rom (rx)  : ORIGIN = 0x00000000, LENGTH =  1M
	ram_local1 (rwx) : ORIGIN = 0x10000000, LENGTH =  128K
	ram_l0dable(rwx) : ORIGIN = 0x10080000, LENGTH = 16K
	ram_m0(rwx) : ORIGIN = 0x10080000, LENGTH = 12K
	ram_local2 (rwx) : ORIGIN = 0x10084000, LENGTH = 48K
	ram_sleep  (rwx) : ORIGIN = 0x10090000, LENGTH = 8K

	ram_m0_stack (rwx) : ORIGIN = 0x10083000, LENGTH =  4K
    /*
	ram_ahb1_spare    (rwx) : ORIGIN = 0x20001000, LENGTH =  3K
	ram_ahb1_m4_share (rwx) : ORIGIN = 0x20001C00, LENGTH =  1K
	ram_ahb1_adchs (rwx)    : ORIGIN = 0x20002000, LENGTH =  4K
	ram_ahb1_m0_share (rwx) : ORIGIN = 0x20003000, LENGTH =  4K
    */

	ram_ahb1_0 (rwx) : ORIGIN = 0x20001000, LENGTH =  4K
	/*ram_ahb1_0 (rwx) : ORIGIN = 0x20004000, LENGTH =  16K */
	/* ram_ahb1_1 (rwx) : ORIGIN = 0x20008000, LENGTH =  16K */
	/* ram_ahb2   (rwx) : ORIGIN = 0x2000C000, LENGTH =  16K */
}

/* cm4_data_share = ORIGIN(ram_ahb1_m4_share);	/* M4 data shared (write by M4 read by M0 */
/* adchs_data     = ORIGIN(ram_ahb1_adchs);	/* M4 ADCHS data (write by ADCHS DMA or M4) */
/* cm0_data_share = ORIGIN(ram_ahb1_m0_share);	/* M0 data shared (write by M0 read by M4) */

cm0_exec_baseaddr = ORIGIN(ram_l0dable);	/* Used by M4 to load code from M4 RAM to M0 final exec/run addr */



/* Enforce emmition of the vector table. */
EXTERN(vector_table)

/* Define the entry point of the output file. */
ENTRY(reset_handler)

/* Define sections. */
SECTIONS
{
	. = ORIGIN(ram_l0dable);

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
	__StackTop = ORIGIN(ram_m0_stack) + LENGTH(ram_m0_stack) - 32;
	PROVIDE(_stack = __StackTop);
}
