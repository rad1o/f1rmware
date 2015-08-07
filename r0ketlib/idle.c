#include <stdint.h>
#include <rad1olib/systick.h>
#include <libopencmsis/core_cm3.h>

/**************************************************************************/

void work_queue(void){
	__WFI();
	return;
}

uint8_t delayms_queue_plus(uint32_t ms, uint8_t final){
    int ret=0;
    int end=_timectr+ms/SYSTICKSPEED;
    do {
            __WFI();
    } while (end >_timectr);
    return ret;
}

void delayms_queue(uint32_t ms){
	int end=_timectr+ms/SYSTICKSPEED;
	do {
            __WFI();
	} while (end >_timectr);
}

void delayms_power(uint32_t ms){
    ms/=SYSTICKSPEED;
    ms+=_timectr;
	do {
        __WFI();
	} while (ms >_timectr);
}

void delayms(uint32_t duration){
	int end=_timectr+duration/SYSTICKSPEED;
	do {
		__WFI();
	} while (end >_timectr);
}
