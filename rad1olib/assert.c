#include <rad1olib/pins.h>
#include <rad1olib/setup.h>
#include <rad1olib/assert.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>

void assert_die(void){
	SETUPgout(LED4);
	while(1){
		TOGGLE(LED4);
		delayNop(3000000);
	};
};
