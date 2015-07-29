#ifndef _SPI_FLASH_H
#define _SPI_FLASH_H 1

#include <stdint.h>

#define FLASH_SIZE (2048*1024)

#define FLASHFS_OFFSET (512*1024)
#define FLASHFS_LENGTH (FLASH_SIZE-FLASHFS_OFFSET)

#define S1_BUSY (1<<0)  /* Busy (volatile) */
#define S1_WEL  (1<<1)  /* Write Enable Latch (volatile) */
#define S1_BP0  (1<<2)  /* Block Protect Bits */
#define S1_BP1  (1<<3)
#define S1_BP2  (1<<4)
#define S1_TB   (1<<5)  /* Top/Bottom protect */
#define S1_SEC  (1<<6)  /* Sector/Block protect */
#define S1_SRP0  (1<<7) /* Status register protect */

#define S2_SRP1 (1<<0)
#define S2_QE   (1<<1)  /* Quad Enable */
//#define reserved  (1<<2)
#define S2_LB1  (1<<3)  /* Security Register Lock Bits */
#define S2_LB2  (1<<4)
#define S2_LB3  (1<<5)
#define S2_CMP  (1<<6)  /* Complement Protect */
#define S2_SUS  (1<<7)  /* Erase/Program Suspent Status (volatile) */

void flash_wait_write();
uint8_t flash_status1(void);
uint8_t flash_status2(void);
void flashInit(void);
void flash_read(uint32_t addr, uint32_t len, uint8_t * data);
void flash_read_sector(uint32_t addr, uint32_t * data);
void flash_write_enable(void);
void flash_program(uint32_t addr, uint32_t len, const uint8_t * data);
void flash_random_program(uint32_t addr, uint32_t len, const uint8_t * data);
void flash_program4(uint32_t addr, uint16_t len, const uint32_t * data);
void flash_usb_program(uint32_t addr, uint16_t len, const uint8_t * data);
void flash_erase(uint32_t addr); /* erase 4k sector */
void flash_write(uint32_t addr, uint16_t len, const uint8_t * data);
void flash_random_write(uint32_t addr, uint16_t len, const uint8_t * data);

#endif /* _SPI_FLASH_H */
