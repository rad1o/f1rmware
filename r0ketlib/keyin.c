#include <stddef.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/idle.h>
#include <rad1olib/systick.h>

#define FUNC (SCU_GPIO_PDN)

static char isTurned;

void inputInit(void){
        /* the GPIO_DIR could be opimized */
        scu_pinmux(KEY_UP_PIN,FUNC|KEY_UP_FUNC);
        GPIO_DIR(KEY_UP_GPORT) &= ~KEY_UP_GPIN;

        scu_pinmux(KEY_DOWN_PIN,FUNC|KEY_DOWN_FUNC);
        GPIO_DIR(KEY_DOWN_GPORT) &= ~KEY_DOWN_GPIN;

        scu_pinmux(KEY_LEFT_PIN,FUNC|KEY_LEFT_FUNC);
        GPIO_DIR(KEY_LEFT_GPORT) &= ~KEY_LEFT_GPIN;

        scu_pinmux(KEY_RIGHT_PIN,FUNC|KEY_RIGHT_FUNC);
        GPIO_DIR(KEY_RIGHT_GPORT) &= ~KEY_RIGHT_GPIN;

        scu_pinmux(KEY_ENTER_PIN,FUNC|KEY_ENTER_FUNC);
        GPIO_DIR(KEY_ENTER_GPORT) &= ~KEY_ENTER_GPIN;
};


void keySetRotation(char doit) {
	isTurned = doit;
}


uint8_t getInputRaw(void) {
    uint8_t result = BTN_NONE;

    if (isTurned) { // display turned to the left
		if (cm3_gpio_get(KEY_UP_GPORT,KEY_UP_GPIN)) {
			result |= BTN_RIGHT;
		}

		if (cm3_gpio_get(KEY_DOWN_GPORT,KEY_DOWN_GPIN)) {
			result |= BTN_LEFT;
		}

		if (cm3_gpio_get(KEY_ENTER_GPORT,KEY_ENTER_GPIN)) {
			result |= BTN_ENTER;
		}

		if (cm3_gpio_get(KEY_LEFT_GPORT,KEY_LEFT_GPIN)) {
			result |= BTN_UP;
		}

		if (cm3_gpio_get(KEY_RIGHT_GPORT,KEY_RIGHT_GPIN)) {
			result |= BTN_DOWN;
		}
	} else {  // as before, the normal way

		if (cm3_gpio_get(KEY_UP_GPORT,KEY_UP_GPIN)) {
			result |= BTN_UP;
		}

		if (cm3_gpio_get(KEY_DOWN_GPORT,KEY_DOWN_GPIN)) {
			result |= BTN_DOWN;
		}

		if (cm3_gpio_get(KEY_ENTER_GPORT,KEY_ENTER_GPIN)) {
			result |= BTN_ENTER;
		}

		if (cm3_gpio_get(KEY_LEFT_GPORT,KEY_LEFT_GPIN)) {
			result |= BTN_LEFT;
		}

		if (cm3_gpio_get(KEY_RIGHT_GPORT,KEY_RIGHT_GPIN)) {
			result |= BTN_RIGHT;
		}
	}

    return result;
}


uint8_t getInput(void) {
    uint8_t key = BTN_NONE;

    key=getInputRaw();
    /* XXX: we should probably debounce the joystick.
            Any ideas how to do this properly?
            For now wait for any release.
     */
    if(key != BTN_NONE)
        while(key==getInputRaw())
            ;

    return key;
}


uint8_t getInputWait(void) {
    uint8_t key;
    while ((key=getInputRaw())==BTN_NONE)
        work_queue();
    delayms_queue(10); /* Delay a little more to debounce */
    return key;
}

uint8_t getInputWaitTimeout(int timeout) {
    uint8_t key;
    if(timeout==0)
        return getInputWait();
    int end=_timectr+timeout/SYSTICKSPEED;
    while ((key=getInputRaw())==BTN_NONE){
        if(_timectr>end)
            break;
        work_queue();
    };
    delayms_queue(10); /* Delay a little more to debounce */
    return key;
}

uint8_t getInputWaitRepeat(void) {
    static uint8_t oldkey=BTN_NONE;
    static int repeatctr=0;
    uint8_t key=getInputRaw();

    if (key != BTN_NONE && key==oldkey){
        int dtime;
        if(!repeatctr)
            dtime=600;
        else if(repeatctr<5)
            dtime=250;
        else if(repeatctr<25)
            dtime=150;
        else if(repeatctr<50)
            dtime=80;
        else
            dtime=20;
        repeatctr++;
        int end=_timectr+(dtime/SYSTICKSPEED);
        while(_timectr<end && key==getInputRaw())
            work_queue();
        key=getInputRaw();
        if (key==oldkey)
            return key;
    };

    repeatctr=0;
    while ((key=getInputRaw())==BTN_NONE){
        work_queue();
    };
    delayms_queue(10); /* Delay a little more to debounce */
    oldkey=key;
    return key;
}

void getInputWaitRelease(void) {
    while (getInputRaw()!=BTN_NONE)
        work_queue();
    delayms_queue(10); /* Delay a little more to debounce */
}
