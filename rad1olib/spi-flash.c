#include <spi-flash.h>
#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/spifi.h>
#include "pins.h"
#include "setup.h"

#define FLASH_SECTOR_SIZE 4096

uint8_t flash_status1(void){
	uint8_t data;

	SPIFI_CMD = SPIFI_CMD_DATALEN(1) | 
		SPIFI_CMD_DOUT(0) |         /* We read data */
		SPIFI_CMD_FIELDFORM(0) |    /* all serial */
		SPIFI_CMD_INTLEN(0) |       /* no. of Intermediate bytes */
		SPIFI_CMD_FRAMEFORM(0x1) |  /* Opcode only */
		SPIFI_CMD_OPCODE(0x05);

	data = SPIFI_DATA_BYTE;

	while ((SPIFI_STAT & SPIFI_STAT_CMD_MASK) >0){
		delay(10); //_WFI();
	};
	return data;
};

uint8_t flash_status2(void){
	uint8_t data;

	SPIFI_CMD = SPIFI_CMD_DATALEN(1) | 
		SPIFI_CMD_DOUT(0) |         /* We read data */
		SPIFI_CMD_FIELDFORM(0) |    /* all serial */
		SPIFI_CMD_INTLEN(0) |       /* no. of Intermediate bytes */
		SPIFI_CMD_FRAMEFORM(0x1) |  /* Opcode only */
		SPIFI_CMD_OPCODE(0x35);

	data = SPIFI_DATA_BYTE;

	while ((SPIFI_STAT & SPIFI_STAT_CMD_MASK) >0){
		delay(10); //_WFI();
	};
	return data;
};

void flash_init(void){
	RESET_CTRL1 = RESET_CTRL1_SPIFI_RST;

	/* Init SPIFI */
	SETUPpin(SPIFI_SCK);
	SETUPpin(SPIFI_SIO3);
	SETUPpin(SPIFI_SIO2);
	SETUPpin(SPIFI_MISO);
	SETUPpin(SPIFI_MOSI);
	SETUPpin(SPIFI_CS);

	/* Use DIV C for flash: 204/3 = 68 MHz
	   the flash could go up to 104, but that produces bit errors */
	CGU_IDIVC_CTRL= CGU_IDIVC_CTRL_CLK_SEL(CGU_SRC_PLL1)
		| CGU_IDIVC_CTRL_AUTOBLOCK(1) 
		| CGU_IDIVC_CTRL_IDIV(3-1)
		| CGU_IDIVC_CTRL_PD(0)
		;
	CGU_BASE_SPIFI_CLK = CGU_BASE_SPIFI_CLK_CLK_SEL(CGU_SRC_IDIVC);

	/* config SPIFI, use defaults */
	SPIFI_CTRL = SPIFI_CTRL_TIMEOUT(0xffff)|SPIFI_CTRL_CSHIGH(0xf)|SPIFI_CTRL_FBCLK(1);

	if (!(flash_status2()&S2_QE)){ /* make sure Quad Enable is set */
		flash_write_enable();
		SPIFI_CMD = SPIFI_CMD_DATALEN(2) | 
			SPIFI_CMD_DOUT(1) |         /* We write data */
			SPIFI_CMD_FIELDFORM(0) |    /* serial */
			SPIFI_CMD_INTLEN(0) |       /* no intermediates */
			SPIFI_CMD_FRAMEFORM(0x1) |  /* Opcode only */
			SPIFI_CMD_OPCODE(0x01);

		SPIFI_DATA_BYTE = 0;      /* S1 */
		SPIFI_DATA_BYTE = S2_QE;  /* S2 */

		while ((SPIFI_STAT & SPIFI_STAT_CMD_MASK) >0){
			delay(10); //_WFI();
		};
	};
};

void flash_read(uint32_t addr, uint32_t len, uint8_t * data){
	uint32_t idx=0;

	SPIFI_ADDR = addr;
//	SPIFI_IDATA= 0x0;  /* We actually don't care about these bits */
	SPIFI_CMD = SPIFI_CMD_DATALEN(len) | 
		SPIFI_CMD_DOUT(0) |         /* We read data */
		SPIFI_CMD_FIELDFORM(2) |    /* opcode normal, rest quad */
		SPIFI_CMD_INTLEN(3) |       /* no. of Intermediate bytes 1+2 */
		SPIFI_CMD_FRAMEFORM(0x4) |  /* Opcode, followed by 3-byte Address */
		SPIFI_CMD_OPCODE(0xeb);

	while (idx<len){
		data[idx++] = SPIFI_DATA_BYTE;
	}

	while ((SPIFI_STAT & SPIFI_STAT_CMD_MASK) >0){
		delay(10); //_WFI();
	};
};

void flash_read_sector(uint32_t addr, uint32_t * data){
	uint32_t idx=0;

	SPIFI_ADDR = addr & ~(FLASH_SECTOR_SIZE-1);
//	SPIFI_IDATA= 0x0;  /* We actually don't care about these bits */
	SPIFI_CMD = SPIFI_CMD_DATALEN(FLASH_SECTOR_SIZE) | 
		SPIFI_CMD_DOUT(0) |         /* We read data */
		SPIFI_CMD_FIELDFORM(2) |    /* opcode normal, rest quad */
		SPIFI_CMD_INTLEN(1) |       /* no. of Intermediate bytes 1 */
		SPIFI_CMD_FRAMEFORM(0x4) |  /* Opcode, followed by 3-byte Address */
		SPIFI_CMD_OPCODE(0xe3);

	while (idx<FLASH_SECTOR_SIZE/sizeof(uint32_t)){
		data[idx++] = SPIFI_DATA;
	}

	while ((SPIFI_STAT & SPIFI_STAT_CMD_MASK) >0){
		delay(10); //_WFI();
	};
};

void flash_write_enable(void){
	SPIFI_CMD = SPIFI_CMD_DATALEN(0) | 
		SPIFI_CMD_FIELDFORM(0) |    /* all serial */
		SPIFI_CMD_INTLEN(0) |       /* no. of Intermediate bytes */
		SPIFI_CMD_FRAMEFORM(0x1) |  /* Opcode only */
		SPIFI_CMD_OPCODE(0x06);

	while ((SPIFI_STAT & SPIFI_STAT_CMD_MASK) >0){
		delay(10); //_WFI();
	};
};

void flash_program(uint32_t addr, uint32_t len, uint8_t * data){
	uint32_t idx=0;

	flash_write_enable();
	SPIFI_ADDR = addr;
	SPIFI_CMD = SPIFI_CMD_DATALEN(len) | 
		SPIFI_CMD_DOUT(1) |         /* We write data */
		SPIFI_CMD_FIELDFORM(1) |    /* opcode/address normal, data quad */
		SPIFI_CMD_INTLEN(0) |       /* no. of Intermediate bytes 1+2 */
		SPIFI_CMD_FRAMEFORM(0x4) |  /* Opcode, followed by 3-byte Address */
		SPIFI_CMD_OPCODE(0x32);

	while (idx<len){
		SPIFI_DATA_BYTE = data[idx++];
	}

	while ((SPIFI_STAT & SPIFI_STAT_CMD_MASK) >0){
		delay(10); //_WFI();
	};
};

void flash_program4(uint32_t addr, uint32_t len, uint32_t * data){
	uint32_t idx=0;

	flash_write_enable();
	SPIFI_ADDR = addr;
	SPIFI_CMD = SPIFI_CMD_DATALEN(len) | 
		SPIFI_CMD_DOUT(1) |         /* We write data */
		SPIFI_CMD_FIELDFORM(1) |    /* opcode/address normal, data quad */
		SPIFI_CMD_INTLEN(0) |       /* no. of Intermediate bytes 1+2 */
		SPIFI_CMD_FRAMEFORM(0x4) |  /* Opcode, followed by 3-byte Address */
		SPIFI_CMD_OPCODE(0x32);

	while (idx<len){
		SPIFI_DATA = data[idx++];
	}

	while ((SPIFI_STAT & SPIFI_STAT_CMD_MASK) >0){
		delay(10); //_WFI();
	};
};

void flash_erase(uint32_t addr){ /* erase 4k sector */
	flash_write_enable();
	SPIFI_ADDR = addr>>12;          /* sector number */
	SPIFI_CMD = SPIFI_CMD_DATALEN(0) | 
		SPIFI_CMD_FIELDFORM(0) |    /* serial */
		SPIFI_CMD_INTLEN(0) |       /* no intermediates*/
		SPIFI_CMD_FRAMEFORM(0x1) |  /* Opcode, followed by 3-byte Address */
		SPIFI_CMD_OPCODE(0x20);

	while ((SPIFI_STAT & SPIFI_STAT_CMD_MASK) >0){
		delay(10); //_WFI();
	};
};

#define gran uint32_t
/* read/modify/write a FLASH_SECTOR_SIZE sector. len should be multiple of 256 */
void flash_write_sector(uint32_t addr, uint16_t len, gran * data){
	uint32_t cache[FLASH_SECTOR_SIZE/sizeof(gran)]; /* maybe Assert SP */
	uint16_t offset = addr&(FLASH_SECTOR_SIZE-1);
	uint16_t idx;
	uint8_t program_needed=0;
	uint8_t erase_needed=0;

	addr &= ~(FLASH_SECTOR_SIZE-1);
	len /= sizeof(gran);

	flash_read_sector(addr,cache);

	for(idx=0;idx<len;idx++){
		if (~cache[idx+offset] & data[idx]){
			erase_needed=1;
			break;
		};
		if (cache[idx+offset] != data[idx]){
			program_needed=1;
		};
	};
	if (erase_needed){
		flash_erase(addr);
		for(idx=0;idx<FLASH_SECTOR_SIZE/sizeof(gran);idx++){
			if(idx>=offset && idx <offset+len){
				cache[idx]=data[idx-offset];
			};
			if (cache[idx]!= ~((gran)0)){
				program_needed=1;
			};
		};
		flash_wait_write();
		if(program_needed){
			flash_program4(addr, FLASH_SECTOR_SIZE/sizeof(gran), cache);
		};
	}else{
		if(program_needed){
			flash_program4(addr+offset,len,data);
			flash_wait_write();
		};
	};
};



void flash_wait_write(){
	while (flash_status1()&S1_BUSY){
		delay(10); // _WFI();
	};
};
