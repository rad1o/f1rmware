#include <r0ketlib/config.h>
#include <rad1olib/setup.h>
#include <rad1olib/battery.h>
#include <rad1olib/pins.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/adc.h>

#include <r0ketlib/keyin.h>

#define SAMPCT (4)
#define threshold GLOBAL(daytrig)
#define RANGE GLOBAL(daytrighyst)

// status: 0=init, 1=decreased 2=increased=ok
uint8_t sensor_status=0;
uint32_t light=150*SAMPCT;
char _isnight=1;

void LightCheck(void){
//    adc_get_single(ADC0,ADC_CR_CH6)*2*330/1023

    if(GLOBAL(chargeled) && batteryCharging()){
        _isnight=1;
        return;
    }

    SETUPadc(RAD1O_LED4);

    uint32_t last_light=light;
    // ADC needs some time?
    light-=light/SAMPCT;
    light += adc_get_single(ADC0,ADC_CR_CH6);
    light-=light/SAMPCT;
    light += adc_get_single(ADC0,ADC_CR_CH6);
    light-=light/SAMPCT;
    light += adc_get_single(ADC0,ADC_CR_CH6);
    light-=light/SAMPCT;
    light += adc_get_single(ADC0,ADC_CR_CH6);

    SETUPgout(RAD1O_LED4);

    // disable light sensor until we get reasonable results
    if(sensor_status<2){
        if(light<last_light) sensor_status=1;
        if(light>last_light && sensor_status==1) sensor_status=2;
        return;
    }

    if(_isnight && light/SAMPCT>(threshold+RANGE))
        _isnight=0;

    if(!_isnight && light/SAMPCT<threshold)
        _isnight=1;
}

uint32_t GetLight(void){
    return light/SAMPCT;
}

char isNight(void){
    return _isnight;
}
