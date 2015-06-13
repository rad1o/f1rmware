#include <stddef.h>
#include "keyin.h"

#define FUNC (SCU_CONF_EPD_EN_PULLDOWN|SCU_CONF_EPUN_DIS_PULLUP)
#define FUNC (SCU_GPIO_PUP)
#define FUNC (SCU_GPIO_PDN)

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


uint8_t getInputRaw(void) {
    uint8_t result = BTN_NONE;

    if (gpio_get(KEY_UP_GPORT,KEY_UP_GPIN)) {
        result |= BTN_UP;
    }

    if (gpio_get(KEY_DOWN_GPORT,KEY_DOWN_GPIN)) {
        result |= BTN_DOWN;
    }

    if (gpio_get(KEY_ENTER_GPORT,KEY_ENTER_GPIN)) {
        result |= BTN_ENTER;
    }

    if (gpio_get(KEY_LEFT_GPORT,KEY_LEFT_GPIN)) {
        result |= BTN_LEFT;
    }

    if (gpio_get(KEY_RIGHT_GPORT,KEY_RIGHT_GPIN)) {
		result |= BTN_RIGHT;
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

