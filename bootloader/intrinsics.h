static inline uint32_t get_sp(void){
	register uint32_t result;
	__asm volatile ("MRS %0, msp\n" : "=r" (result) );
	return result;
};


static inline void boot(const void * vtable){
	// Set new Vtable
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

extern void * _text_size;
extern void * _reloc_ep;
extern void * _text_start;

extern unsigned _bss, _ebss;

int main(void);

void __attribute__ ((naked)) reset_handler(void) {
	volatile unsigned *dest;
	volatile uint32_t idx;

	if ((void *)CREG_M4MEMMAP != &_reloc_ep){
		/* Move ourselves to _reloc_ep and restart there */
		for (idx=0; idx < ((uintptr_t)& _text_size)/sizeof(uint32_t); idx++){
			((uint32_t*)&_reloc_ep)[idx]= ((uint32_t *)&_text_start)[idx];
		};
		CREG_M4MEMMAP = (uintptr_t)&_reloc_ep;
		boot(&_reloc_ep);
	};

	/* Initialize BSS */
	for (dest = &_bss; dest < &_ebss; ) {
		*dest++ = 0;
	}

	/* Call the application's entry point. */
	main();
}
