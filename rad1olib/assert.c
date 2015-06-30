#include <pins.h>
#include <setup.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>

void assert_die(void){
	SETUPgout(LED4);
	while(1){
		TOGGLE(LED4);
		delay(3000000);
	};
};
