#include <r0ketlib/config.h>
#include <rad1olib/setup.h>
#include <rad1olib/battery.h>
#include <rad1olib/pins.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/adc.h>

#include <r0ketlib/keyin.h>




#define SAMPCT (4)
uint32_t light=150*SAMPCT;
char _isnight=1;

#define threshold GLOBAL(daytrig)
#define RANGE GLOBAL(daytrighyst)

void LightCheck(void){
//    adc_get_single(ADC0,ADC_CR_CH6)*2*330/1023

    if(GLOBAL(chargeled) && batteryCharging()){
        _isnight=1;
        return;
    }

    SETUPadc(LED4);

    // ADC needs some time?
    light-=light/SAMPCT;
    light += adc_get_single(ADC0,ADC_CR_CH6);
    light-=light/SAMPCT;
    light += adc_get_single(ADC0,ADC_CR_CH6);
    light-=light/SAMPCT;
    light += adc_get_single(ADC0,ADC_CR_CH6);
    light-=light/SAMPCT;
    light += adc_get_single(ADC0,ADC_CR_CH6);

    if(_isnight && light/SAMPCT>(threshold+RANGE))
        _isnight=0;

    if(!_isnight && light/SAMPCT<threshold)
        _isnight=1;

    SETUPgout(LED4);
}

uint32_t GetLight(void){
    return light/SAMPCT;
}

char isNight(void){
    return _isnight;
}
