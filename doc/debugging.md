## Debugging
There is no easy way to debug the code while it runs on the board, but the board has 4 LEDs which can be used to display the current status of a program.

    #include "pins.h"
    ...
    #define LED1 P4_1, SCU_CONF_FUNCTION0, GPIO2, GPIOPIN1, clear
    #define LED2 P4_2, SCU_CONF_FUNCTION0, GPIO2, GPIOPIN2, clear
    #define LED3 P6_12, SCU_CONF_FUNCTION0, GPIO2, GPIOPIN8, clear
    #define LED4 PB_6, SCU_CONF_FUNCTION4, GPIO5, GPIOPIN26, clear
    ...
    SETUPgout(LED1);
    SETUPgout(LED2);
    SETUPgout(LED3);
    SETUPgout(LED4);
    ...
    TOGGLE(LED1);
    TOGGLE(LED2);
    TOGGLE(LED3);
    TOGGLE(LED4);
