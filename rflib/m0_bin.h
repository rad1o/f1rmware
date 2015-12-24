#ifndef __M0_BIN_H
#define __M0_BIN_H
extern uint8_t m0_bin[];
extern uint32_t m0_bin_size;
/* we should probably get this from the linker instead */
const void *cm0_exec_baseaddr = (const void*) 0x20000000;
#endif
