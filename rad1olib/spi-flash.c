#include <rad1olib/spi-flash.h>
#include <rad1olib/pins.h>
#include <rad1olib/setup.h>
#include <rad1olib/assert.h>
#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/spifi.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>

#define FLASH_SECTOR_SIZE 4096

/*                        opc   datalen, dout, fieldform, intlen, frameform  */
/*                                             0: serial          1: op only */
/*                                             1: q data          2: op + 1  */
/*                                             2: op serial       3: op + 2  */
/*                                                                4: op + 3  */

#define OP_WRITE_ENABLE   0x06, 0,       0,    0,         0,      1

#define OP_READ_S1        0x05, 1,       0,    0,         0,      1
#define OP_READ_S2        0x35, 1,       0,    0,         0,      1
#define OP_WRITE_ST       0x01, 2,       1,    0,         0,      1

#define OP_READ           0x0B, /*x*/0,  0,    0,         1,      1+3
#define OP_QREAD          0xEB, /*x*/0,  0,    2,         3,      1+3
#define OP_QREAD_2        0xE7, /*x*/0,  0,    2,         2,      1+3
#define OP_QREAD_8        0xE3, /*x*/0,  0,    2,         1,      1+3

#define OP_PROGRAM        0x02, /*x*/0,  1,    0,         0,      1+3
#define OP_QPROGRAM       0x32, /*x*/0,  1,    1,         0,      1+3

#define OP_SECT_ERASE_4K  0x20, 0,       0,    0,         0,      1+3
#define OP_SECT_ERASE_32K 0x52, 0,       0,    0,         0,      1+3
#define OP_SECT_ERASE_64K 0xD8, 0,       0,    0,         0,      1+3
#define OP_CHIP_ERASE     0xC7, 0,       0,    0,         0,      1

#define OP_DEVICE_ID      0x92, 2,       0,    0,         1,      1+3
#define OP_DEVICE_ID_Q    0x94, 2,       0,    2,         3,      1+3
#define OP_UNIQUE_ID      0x4B, 8,       0,    0,         4,      1
#define OP_JEDEC_ID       0x9F, 3,       0,    0,         0,      1

#define MK_SPIFI_CMD_2(opcode,datalen,dout,fieldform,intlen,frameform) \
	SPIFI_CMD_DATALEN(datalen)     | \
	SPIFI_CMD_DOUT(dout)           | \
	SPIFI_CMD_FIELDFORM(fieldform) | \
	SPIFI_CMD_INTLEN(intlen)       | \
	SPIFI_CMD_FRAMEFORM(frameform) | \
	SPIFI_CMD_OPCODE(opcode)

#define MK_SPIFI_CMD(args...) MK_SPIFI_CMD_2(args)

static void wait_spifi(void){
	while ((SPIFI_STAT & SPIFI_STAT_CMD_MASK) >0){
		delayNop(10); //_WFI();
	};
}

void flash_wait_write(){
	while (flash_status1() & S1_BUSY){
		delayNop(10); // _WFI();
	};
}

uint8_t flash_status1(void){
	uint8_t data;

	SPIFI_CMD = MK_SPIFI_CMD(OP_READ_S1);

	data = SPIFI_DATA_BYTE;

	wait_spifi();

	return data;
}

uint8_t flash_status2(void){
	uint8_t data;

	SPIFI_CMD = MK_SPIFI_CMD(OP_READ_S2);

	data = SPIFI_DATA_BYTE;

	wait_spifi();

	return data;
};

void flashInit(void){
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
		SPIFI_CMD = MK_SPIFI_CMD(OP_WRITE_ST);

		SPIFI_DATA_BYTE = 0;      /* S1 */
		SPIFI_DATA_BYTE = S2_QE;  /* S2 */

		wait_spifi();
	};
}

void flash_read(uint32_t addr, uint32_t len, uint8_t * data){
	uint32_t idx=0;

	SPIFI_ADDR = addr;
//	SPIFI_IDATA= 0x0;  /* We actually don't care about these bits */
	SPIFI_CMD = MK_SPIFI_CMD(OP_QREAD) | SPIFI_CMD_DATALEN(len);

	while (idx<len){
		data[idx++] = SPIFI_DATA_BYTE;
	}

	wait_spifi();
};

void flash_read_sector(uint32_t addr, uint32_t * data){
	uint32_t idx=0;

	SPIFI_ADDR = addr & ~(FLASH_SECTOR_SIZE-1);
	SPIFI_CMD = MK_SPIFI_CMD(OP_QREAD_8) | SPIFI_CMD_DATALEN(FLASH_SECTOR_SIZE);

	while (idx<FLASH_SECTOR_SIZE/sizeof(uint32_t)){
		data[idx++] = SPIFI_DATA;
	}

	wait_spifi();
}

void flash_write_enable(void){
	SPIFI_CMD = MK_SPIFI_CMD(OP_WRITE_ENABLE);
	wait_spifi();
};

void flash_program(uint32_t addr, uint32_t len, const uint8_t * data){
	uint32_t idx=0;

	flash_write_enable();
	SPIFI_ADDR = addr;
	SPIFI_CMD = MK_SPIFI_CMD(OP_QPROGRAM)|SPIFI_CMD_DATALEN(len);

	while (idx<len){
		SPIFI_DATA_BYTE = data[idx++];
	}

	wait_spifi();
}

#define FLASH_PROGRAM_SIZE 256

void flash_random_program(uint32_t addr, uint32_t len, const uint8_t * data){
	uint16_t cut=0;
	uint16_t offset = addr &  (FLASH_PROGRAM_SIZE-1);

	/*
	lcdPrint("p:");
	lcdPrint(IntToStr(addr,8,F_HEX));
	lcdPrint(" ");
	lcdPrint(IntToStr(len,4,F_HEX));
	lcdPrintln(" ");
	lcdPrint("-:");
	lcdPrint(IntToStr(data,8,F_HEX));
	lcdPrint(" ");
	lcdPrint(IntToStr(data[0],2,F_HEX));
	lcdPrintln(" ");
	lcdDisplay(); */

	while (offset+len> FLASH_PROGRAM_SIZE){
		cut=FLASH_PROGRAM_SIZE-offset;
		flash_program(addr,cut,data);
		flash_wait_write();
		addr+=cut;
		offset=0;
		len-=cut;
		data+=cut;
	};
	flash_program(addr,cut,data);
	flash_wait_write();
}

void flash_program4(uint32_t addr, uint16_t len, const uint32_t * data){
	uint32_t idx=0;

	flash_write_enable();
	SPIFI_ADDR = addr;
	SPIFI_CMD = MK_SPIFI_CMD(OP_QPROGRAM)|SPIFI_CMD_DATALEN(len);

	while (idx<len){
		SPIFI_DATA = data[idx++];
	}
	wait_spifi();
};

void flash_usb_program(uint32_t addr, uint16_t len, const uint8_t * data){
	ASSERT(addr%FLASH_PROGRAM_SIZE==0);
	ASSERT(((uintptr_t)data)%4==0);
	ASSERT(len==512);

	flash_program4(addr,
			FLASH_PROGRAM_SIZE/sizeof(uint32_t),
			(uint32_t *)data);
	flash_program4(addr+FLASH_PROGRAM_SIZE,
			FLASH_PROGRAM_SIZE/sizeof(uint32_t),
			(uint32_t *)(data+FLASH_PROGRAM_SIZE));
};

void flash_erase(uint32_t addr){ /* erase 4k sector */
	flash_write_enable();
	SPIFI_ADDR = addr;
	SPIFI_CMD = MK_SPIFI_CMD(OP_SECT_ERASE_4K);

	wait_spifi();
}

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
		program_needed=0;
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
			flash_wait_write();
		};
	}else{
		if(program_needed){
			flash_program4(addr+offset,len,data);
			flash_wait_write();
		};
	};
}
#undef gran

void flash_write(uint32_t addr, uint16_t len, const uint8_t * data){
	/* assumes buffer does not straddle sector boundary */
	uint8_t cache[FLASH_SECTOR_SIZE]; /* maybe Assert SP */
	uint16_t offset = addr&(FLASH_SECTOR_SIZE-1);
	uint16_t idx;
	uint8_t program_needed=0;
	uint8_t erase_needed=0;

	addr &= ~(FLASH_SECTOR_SIZE-1);

	flash_read(addr,FLASH_SECTOR_SIZE,cache);

	for(idx=0;idx<len;idx++){
		if (~cache[idx+offset] & data[idx]){
			erase_needed=1;
			break;
		};
		if (cache[idx+offset] != data[idx]){
			program_needed=1;
		};
	};
/*	lcdPrint("P:");
	lcdPrint(IntToStr(addr,8,F_HEX));
	lcdPrint(" ");
	lcdPrint(IntToStr(len,4,F_HEX));
	lcdPrintln(" ");
	lcdPrint("+:");
	lcdPrint(IntToStr(data,8,F_HEX));
	lcdPrint(" ");
	lcdPrint(IntToStr(data[0],2,F_HEX));
	lcdPrintln(" ");
	lcdDisplay(); */
	if (erase_needed){
		program_needed=0;
		flash_erase(addr);
		for(idx=0;idx<FLASH_SECTOR_SIZE;idx++){
			if(idx>=offset && idx <offset+len){
				cache[idx]=data[idx-offset];
			};
			if (cache[idx]!= ~((uint8_t)0)){
				program_needed=1;
			};
		};
		flash_wait_write();
		if(program_needed){
			flash_random_program(addr, FLASH_SECTOR_SIZE, cache);
			flash_wait_write();
		};
	}else{
		if(program_needed){
			flash_random_program(addr+offset,len,data);
			flash_wait_write();
		};
	};
}

void flash_random_write(uint32_t addr, uint16_t len, const uint8_t * data){
	uint16_t offset = addr&(FLASH_SECTOR_SIZE-1);
	uint16_t cut;
	while(offset+len > FLASH_SECTOR_SIZE){
		cut=FLASH_SECTOR_SIZE-offset;
		flash_write(addr,cut,data);
		addr+=cut;
		offset=0;
		len-=cut;
		data+=cut;
	};
	flash_write(addr,len,data);
};
