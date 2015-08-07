#include <stdint.h>
#define LOCAL
#include <rad1olib/systick.h>
#undef LOCAL
#include <libopencm3/cm3/systick.h>

volatile uint32_t _timectr=0;

void systickInit(){
	systick_set_clocksource(0);
	systick_set_reload(12e6/SYSTICKSPEED/1000);
	systick_interrupt_enable();
	systick_counter_enable();
};

void systickAdjustFreq(uint32_t hz){
	systick_set_reload(hz/SYSTICKSPEED/1000);
};

void systickDisable(){
	systick_interrupt_disable();
	systick_counter_disable();
};
