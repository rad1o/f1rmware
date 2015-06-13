#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>

//down
#define BY_AMP_PIN                  (P1_7)
#define BY_AMP_FUNC                 (SCU_CONF_FUNCTION0)
#define BY_AMP_GPORT                GPIO1
#define BY_AMP_GPIN                 GPIOPIN0

#define BY_AMP_N_PIN                (P2_5)
#define BY_AMP_N_FUNC                 (SCU_CONF_FUNCTION4)
#define BY_AMP_N_GPORT                GPIO5
#define BY_AMP_N_GPIN                 GPIOPIN5
 
//left
#define TX_RX_PIN                   (P2_10)
#define TX_RX_FUNC                 (SCU_CONF_FUNCTION0)
#define TX_RX_GPORT                GPIO0
#define TX_RX_GPIN                 GPIOPIN14
#define TX_RX_N_PIN                 (P2_11)
#define TX_RX_N_FUNC                 (SCU_CONF_FUNCTION0)
#define TX_RX_N_GPORT                GPIO1
#define TX_RX_N_GPIN                 GPIOPIN11
 
//up
#define BY_MIX_PIN                  (P2_12)
#define BY_MIX_FUNC                 (SCU_CONF_FUNCTION0)
#define BY_MIX_GPORT                GPIO1
#define BY_MIX_GPIN                 GPIOPIN12
 
//right
#define LOW_HIGH_FILT_PIN           (P5_2)
#define LOW_HIGH_FILT_FUNC                 (SCU_CONF_FUNCTION0)
#define LOW_HIGH_FILT_GPORT                GPIO2
#define LOW_HIGH_FILT_GPIN                 GPIOPIN11
#define LOW_HIGH_FILT_N_PIN			(P5_3)
#define LOW_HIGH_FILT_N_FUNC                 (SCU_CONF_FUNCTION0)
#define LOW_HIGH_FILT_N_GPORT                GPIO2
#define LOW_HIGH_FILT_N_GPIN                 GPIOPIN12

//down2
#define TX_AMP_PIN                  (P5_6)
#define TX_AMP_FUNC                 (SCU_CONF_FUNCTION0)
#define TX_AMP_GPORT                GPIO2
#define TX_AMP_GPIN                 GPIOPIN15
#define RX_LNA_PIN                  (P6_7)
#define RX_LNA_FUNC                 (SCU_CONF_FUNCTION4)
#define RX_LNA_GPORT                GPIO5
#define RX_LNA_GPIN                 GPIOPIN15
 
//left2
#define CS_VCO_PIN   (P5_4)
#define CS_VCO_FUNC  (SCU_CONF_FUNCTION0)
#define CS_VCO_GPORT GPIO2
#define CS_VCO_GPIN  GPIOPIN13



void feldInit();
