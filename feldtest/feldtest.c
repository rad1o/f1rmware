#include <stddef.h>
#include "feldtest.h"

#define FUNC (SCU_GPIO_NOPULL)

void feldInit(){

scu_pinmux(BY_AMP_PIN,FUNC|BY_AMP_FUNC);
GPIO_DIR(BY_AMP_GPORT) |= BY_AMP_GPIN;

scu_pinmux(BY_AMP_N_PIN,FUNC|BY_AMP_N_FUNC);
GPIO_DIR(BY_AMP_N_GPORT) |= BY_AMP_N_GPIN;

scu_pinmux(TX_AMP_PIN,FUNC|BY_AMP_FUNC);
GPIO_DIR(TX_AMP_GPORT) |= TX_AMP_GPIN;

scu_pinmux(RX_LNA_PIN,FUNC|RX_LNA_FUNC);
GPIO_DIR(RX_LNA_GPORT) |= RX_LNA_GPIN;

scu_pinmux(TX_RX_PIN,FUNC|TX_RX_FUNC);
GPIO_DIR(TX_RX_GPORT) |= TX_RX_GPIN;

scu_pinmux(TX_RX_N_PIN,FUNC|TX_RX_N_FUNC);
GPIO_DIR(TX_RX_N_GPORT) |= TX_RX_N_GPIN;

scu_pinmux(BY_MIX_PIN,FUNC|BY_MIX_FUNC);
GPIO_DIR(BY_MIX_GPORT) |= BY_MIX_GPIN;

scu_pinmux(LOW_HIGH_FILT_PIN,FUNC|LOW_HIGH_FILT_FUNC);
GPIO_DIR(LOW_HIGH_FILT_GPORT) |= LOW_HIGH_FILT_GPIN;

scu_pinmux(LOW_HIGH_FILT_N_PIN,FUNC|LOW_HIGH_FILT_N_FUNC);
GPIO_DIR(LOW_HIGH_FILT_N_GPORT) |= LOW_HIGH_FILT_N_GPIN;

scu_pinmux(CS_VCO_PIN,FUNC|CS_VCO_FUNC);
GPIO_DIR(CS_VCO_GPORT) |= CS_VCO_GPIN;

#define SETUP(foo) scu_pinmux(foo ## _PIN,FUNC|foo ## _FUNC); \
					GPIO_DIR(foo ## _GPORT) |= foo ## _GPIN;


SETUP(MIXER_EN);

					gpio_clear(BY_MIX_GPORT,BY_MIX_GPIN);

					gpio_clear(BY_AMP_GPORT,BY_AMP_GPIN);
					gpio_set(BY_AMP_N_GPORT,BY_AMP_N_GPIN);

					gpio_clear(TX_RX_GPORT,TX_RX_GPIN);
					gpio_set(TX_RX_N_GPORT,TX_RX_N_GPIN);

					gpio_clear(LOW_HIGH_FILT_GPORT,LOW_HIGH_FILT_GPIN);
					gpio_set(LOW_HIGH_FILT_N_GPORT,LOW_HIGH_FILT_N_GPIN);

					gpio_clear(TX_AMP_GPORT,TX_AMP_GPIN);
					gpio_clear(RX_LNA_GPORT,RX_LNA_GPIN);

					gpio_clear(CS_VCO_GPORT,CS_VCO_GPIN);

#define OFF(foo) gpio_clear(foo ## _GPORT,foo ## _GPIN);
					OFF(MIXER_EN);

};
