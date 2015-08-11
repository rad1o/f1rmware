#include <rad1olib/pins.h>

//#include <rad1olib/setup.h>
//#include <r0ketlib/display.h>

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/adc.h>

static uint32_t voltage=4000;

void batteryVoltageCheck(void)
{
    voltage = adc_get_single(ADC0,ADC_CR_CH3)*2*3300/1023;

    //battery is assumed empty if the voltage falls bellow 3.5V
    if( voltage < 3500 ){
        //put the chip into deep power down
        //SCB_SCR |= SCB_SCR_SLEEPDEEP;
        //PMU_PMUCTRL = PMU_PMUCTRL_DPDEN_DEEPPOWERDOWN;
        //__asm volatile ("WFI");
    };
}

uint32_t batteryGetVoltage(void)
{
    return voltage;
}

bool batteryCharging(void)
{
    // The battery charging indicator pin is active low
    return GET(BC_IND) == 0;
}

void batteryInit(void)
{
    SETUPgin(BC_DONE);
    SETUPgout(BC_CEN);
    SETUPgout(BC_PEN2);
    SETUPgout(BC_USUS);
    SETUPgout(BC_THMEN);
    SETUPgin(BC_IND);
    SETUPgin(BC_OT);
    SETUPgin(BC_DOK);
    SETUPgin(BC_UOK);
    SETUPgin(BC_FLT);
}

