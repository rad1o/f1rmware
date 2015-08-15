#ifndef _KEYIN_H
#define _KEYIN_H 1

#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>

#define BTN_NONE 0
#define BTN_UP   (1<<0)
#define BTN_DOWN (1<<1)
#define BTN_LEFT (1<<2)
#define BTN_RIGHT (1<<3)
#define BTN_ENTER (1<<4)

void keySetRotation(char doit);
uint8_t getInputRaw(void);
uint8_t getInput(void);
void inputInit(void);
uint8_t getInputWait(void);
uint8_t getInputWaitTimeout(int timeout);
uint8_t getInputWaitRepeat(void);
void getInputWaitRelease(void);

/* XXX: probably should be in central header file */
#define KEY_DOWN_PIN     (PB_0)
#define KEY_DOWN_FUNC    (SCU_CONF_FUNCTION4)
#define KEY_DOWN_GPORT   GPIO5
#define KEY_DOWN_GPIN    GPIOPIN20

#define KEY_UP_PIN     (PB_1)
#define KEY_UP_FUNC    (SCU_CONF_FUNCTION4)
#define KEY_UP_GPORT   GPIO5
#define KEY_UP_GPIN    GPIOPIN21

#define KEY_LEFT_PIN     (PB_2)
#define KEY_LEFT_FUNC    (SCU_CONF_FUNCTION4)
#define KEY_LEFT_GPORT   GPIO5
#define KEY_LEFT_GPIN    GPIOPIN22

#define KEY_RIGHT_PIN     (PB_3)
#define KEY_RIGHT_FUNC    (SCU_CONF_FUNCTION4)
#define KEY_RIGHT_GPORT   GPIO5
#define KEY_RIGHT_GPIN    GPIOPIN23

#define KEY_ENTER_PIN     (PB_4)
#define KEY_ENTER_FUNC    (SCU_CONF_FUNCTION4)
#define KEY_ENTER_GPORT   GPIO5
#define KEY_ENTER_GPIN    GPIOPIN24

#endif /* _KEYIN_H */
