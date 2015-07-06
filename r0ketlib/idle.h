#ifndef _IDLE_H
#define _IDLE_H 1

void work_queue(void);
uint8_t delayms_queue_plus(uint32_t ms, uint8_t final);
void delayms_queue(uint32_t ms);
void delayms_power(uint32_t ms);

#define SYSTICKSPEED 1
extern volatile uint32_t _timectr;

#endif /* _IDLE_H */
