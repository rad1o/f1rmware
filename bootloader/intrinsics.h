static inline uint32_t get_sp(void){
	register uint32_t result;
	__asm volatile ("MRS %0, msp\n" : "=r" (result) );
	return result;
};

static inline void boot(const void * vtable){
	// Set new Vtable
	CREG_M4MEMMAP = (uintptr_t)vtable;
	SCB_VTOR = (uintptr_t) vtable;  

	// Reset stack pointer & branch to the new reset vector.  
	__asm(  "mov r0, %0\n"  
			"ldr sp, [r0]\n"  
			"ldr r0, [r0, #4]\n"  
			"bx r0\n"  
			:  
			: "r"(vtable)
			: "%sp", "r0");  
};

static inline uint32_t get_pc(void){
	register uint32_t result;
	__asm volatile ("mov %0, pc\n" : "=r" (result) );
	return result;
};

extern void * _bin_size;
extern void * _reloc_ep;
extern void * _text_start;

extern unsigned _bss, _ebss;

int main(uint32_t);

void __attribute__ ((naked)) __attribute((section(".reset")))reset_handler(void) {
	volatile unsigned *dest;
	volatile uint32_t idx;
	static uint32_t startloc=-1; /* initialize so it is not in BSS */

	if ((void *)CREG_M4MEMMAP != &_reloc_ep){
		/* Move ourselves to _reloc_ep and restart there */
		for (idx=0; idx < ((uintptr_t)& _bin_size)/sizeof(uint32_t); idx++){
			((uint32_t*)&_reloc_ep)[idx]= ((uint32_t *)&_text_start)[idx];
		};
		/* remember where we started. Needs to be done after the copy of data */
		startloc=CREG_M4MEMMAP;
		/* set shadow area to new code loction, so boot() still works */
		CREG_M4MEMMAP = (uintptr_t)&_reloc_ep;
		boot(&_reloc_ep);
	};

	/* Initialize BSS */
	for (dest = &_bss; dest < &_ebss; ) {
		*dest++ = 0;
	}

	/* Call the application's entry point. */
	main(startloc);
}
