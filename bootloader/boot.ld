/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2012 Michael Ossmann <mike@ossmann.com>
 * Copyright (C) 2012 Benjamin Vernoux <titanmkd@gmail.com>
 * Copyright (C) 2012 Jared Boone <jared@sharebrained.com>
 * Copyright (C) 2015 Stefan `Sec` Zehl <sec@42.org>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Boot linker script for LPC43XX target using libopencm3. */

/* Memory regions must be defined in the ld script which includes this one. */

/* Enforce emmission of the vector table. */
EXTERN(vector_table)

/* Define the entry point of the output file. */
ENTRY(reset_handler)

/* Define sections. */
SECTIONS
{
	.vtors : {
		_text_start = .;
		. = ALIGN(0x400);
		*(.vectors)	/* Vector table */
		*(.reset)	/* reset hdlr */
		. = ALIGN(64);  /* XXX: needs to match alignment of .text */
	} >shadow

	.stext (NOLOAD) : { /* hack to advance ">ram_local2" to the current offset */
		_reloc_ep = .;
		. += SIZEOF(.vtors);
	} >ram_local2

	.text : {
		*(.text*)	/* Program code */
		. = ALIGN(4);
	} >ram_local2 AT >shadow


	.rodata : {
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
	} >ram_local2 AT >shadow

	/* C++ Static constructors/destructors, also used for __attribute__
	 * ((constructor)) and the likes */
	.preinit_array : {
		. = ALIGN(4);
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
	} >ram_local2 AT >shadow

	.init_array : {
		. = ALIGN(4);
		__init_array_start = .;
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = .;
	} >ram_local2 AT >shadow

	.fini_array : {
		. = ALIGN(4);
		__fini_array_start = .;
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		__fini_array_end = .;
	} >ram_local2 AT >shadow

	/*
	 * Another section used by C++ stuff, appears when using newlib with
	 * 64bit (long long) printf support
	 */
	.ARM.extab : {
		*(.ARM.extab*)
	} >ram_local2 AT >shadow
    
	/* exception index - required due to libgcc.a issuing /0 exceptions */
	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
	} >ram_local2 AT >shadow

	.data : {
		_data = .;
		*(.data*)	/* Read-write initialized data */
		. = ALIGN(4);
		_edata = .;
	} >ram_local2 AT >shadow

	.bss : {
		_bss = .;
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >ram_local2 AT >shadow

	/* exception unwind data - required due to libgcc.a issuing /0 exceptions */
	.ARM.extab : {
		*(.ARM.extab*)
	} >ram_local2 AT >shadow

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }

	. = ALIGN(4);

	_end = .;
	_size = _end - _reloc_ep;

	.dummy : {
		_bin_end  = .;
	} > shadow

	_bin_size = (_bin_end - _text_start);

	/* Leave room above stack for IAP to run. */
	__StackTop = ORIGIN(ram_local2) + LENGTH(ram_local2) - 32;
	PROVIDE(_stack = __StackTop);
	_app_start = ORIGIN(ram_local1);
}
