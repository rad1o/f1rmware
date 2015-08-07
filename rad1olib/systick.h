#ifndef _SYSTICK_H
#define _SYSTICK_H 1

// Note:
// Our time implementation will fail after 497 days of continous uptime.
// ( 2^32 / 1000 * SYSTICKSPEED ) seconds

#define SYSTICKSPEED 1
#ifndef LOCAL
extern volatile uint32_t _timectr;
#endif

void systickInit();
void systickAdjustFreq(uint32_t hz);
void systickDisable();

#define incTimer(void) do{_timectr++;}while(0);
#define getTimer() (_timectr)

#endif /* _SYSTICK_H */
