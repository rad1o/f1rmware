#ifndef _PIN_H
#define _PIN_H 1

#define _PIN(pin, func, ...) pin
#define _FUNC(pin, func, ...) func
#define _GPORT(pin, func, gport, gpin, ...) gport
#define _GPIN(pin, func, gport, gpin, ...) gpin
#define _GPIO(pin, func, gport, gpin, ...) gport,gpin
#define _VAL(pin, func, gport, gpin, val, ...) val

#define PASTER(x) gpio_ ## x
#define WRAP(x) PASTER(x)

#define SETUPadc(args...)  scu_pinmux(_PIN(args),SCU_CONF_EPUN_DIS_PULLUP|_FUNC(args)); GPIO_DIR(_GPORT(args)) &= ~ _GPIN(args); SCU_ENAIO0|=SCU_ENAIO_ADCx_6;
#define SETUPgin(args...)  scu_pinmux(_PIN(args),_FUNC(args)); GPIO_DIR(_GPORT(args)) &= ~ _GPIN(args);
#define SETUPgout(args...) scu_pinmux(_PIN(args),SCU_CONF_EPUN_DIS_PULLUP|SCU_CONF_EZI_EN_IN_BUFFER|_FUNC(args)); GPIO_DIR(_GPORT(args)) |= _GPIN(args); WRAP( _VAL(args) ) (_GPIO(args));
#define SETUPpin(args...)  scu_pinmux(_PIN(args),_FUNC(args))

#define TOGGLE(x) gpio_toggle(_GPIO(x))
#define OFF(x...) gpio_clear(_GPIO(x))
#define ON(x...)  gpio_set(_GPIO(x))
#define GET(x...) gpio_get(_GPIO(x))


// Pull: SCU_GPIO_NOPULL, SCU_GPIO_PDN, SCU_GPIO_PUP

#define LCD_BL_EN   P1_1,  SCU_CONF_FUNCTION0, GPIO0, GPIOPIN8,  set          // LCD Backlight
#define LCD_BL_EN_  P1_1,  SCU_CONF_FUNCTION1, GPIO0, GPIOPIN8				  // LCD Backlight: PWM

#define LCD_CS      P9_0,  SCU_CONF_FUNCTION0, GPIO4, GPIOPIN12, set 
#define LCD_RESET   P9_4,  SCU_CONF_FUNCTION4, GPIO5, GPIOPIN17, clear

#define LCD_MOSI    P1_4,  SCU_CONF_FUNCTION5|SCU_SSP_IO
#define LCD_SCK     P1_19, SCU_CONF_FUNCTION1|SCU_SSP_IO

#define LCD_SSP     SSP1_NUM

#define EN_VDD      P5_0,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN9,  clear        // RF Power
#define EN_1V8      P6_10, SCU_CONF_FUNCTION0, GPIO3, GPIOPIN6,  clear        // CPLD Power
#define MIC_AMP_DIS P9_1,  SCU_CONF_FUNCTION0, GPIO4, GPIOPIN13, set          // MIC Power

#define LED1        P4_1,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN1,  clear
#define LED2        P4_2,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN2,  clear
#define LED3        P6_12, SCU_CONF_FUNCTION0, GPIO2, GPIOPIN8,  clear
#define LED4        PB_6,  SCU_CONF_FUNCTION4, GPIO5, GPIOPIN26, clear
#define RGB_LED     P8_0,  SCU_CONF_FUNCTION0, GPIO4, GPIOPIN0,  clear

	/* input */
#define BC_DONE     PD_16, SCU_GPIO_PUP|SCU_CONF_FUNCTION4, GPIO6, GPIOPIN30  // Charge Complete Output (active low)
	/*output */
#define BC_CEN      PA_3,  SCU_CONF_FUNCTION0, GPIO4, GPIOPIN10, clear        // Active-Low Charger Enable Input
#define BC_PEN2     PA_4,  SCU_CONF_FUNCTION4, GPIO5, GPIOPIN19, set          // Input Limit Control 2. (100mA/475mA)
#define BC_USUS     PD_12, SCU_CONF_FUNCTION4, GPIO6, GPIOPIN26, clear        // (active low) USB Suspend Digital Input (disable charging)
#define BC_THMEN    P4_0,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN0,  clear        // Thermistor Enable Input / XXX: not connected on Proto1
	/* input */
#define BC_IND      PD_11, SCU_GPIO_PUP|SCU_CONF_FUNCTION4, GPIO6, GPIOPIN25  // (active low) Charger Status Output
#define BC_OT       P8_7,  SCU_GPIO_PUP|SCU_CONF_FUNCTION0, GPIO4, GPIOPIN7   // (active low) Battery Overtemperature Flag
#define BC_DOK      P8_6,  SCU_GPIO_PUP|SCU_CONF_FUNCTION0, GPIO4, GPIOPIN6   // (active low) DC Power-OK Output
#define BC_UOK      P8_5,  SCU_GPIO_PUP|SCU_CONF_FUNCTION0, GPIO4, GPIOPIN5   // (active low) USB Power-OK Output
#define BC_FLT      P8_4,  SCU_GPIO_PUP|SCU_CONF_FUNCTION0, GPIO4, GPIOPIN4   // (active low) Fault Output


/* RF config */
#define BY_AMP		    P1_7,  SCU_CONF_FUNCTION0, GPIO1, GPIOPIN0,  clear
#define BY_AMP_N	    P2_5,  SCU_CONF_FUNCTION4, GPIO5, GPIOPIN5,  clear
#define TX_RX		    P2_10, SCU_CONF_FUNCTION0, GPIO0, GPIOPIN14, clear
#define TX_RX_N		    P2_11, SCU_CONF_FUNCTION0, GPIO1, GPIOPIN11, clear
#define BY_MIX		    P2_12, SCU_CONF_FUNCTION0, GPIO1, GPIOPIN12, clear
#define BY_MIX_N	    P5_1,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN10, clear
#define LOW_HIGH_FILT   P5_2,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN11, clear
#define LOW_HIGH_FILT_N P5_3,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN12, clear
#define TX_AMP		    P5_6,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN15, clear
#define RX_LNA		    P6_7,  SCU_CONF_FUNCTION4, GPIO5, GPIOPIN15, clear
#define CE_VCO		    P5_4,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN13, clear   // Chip Enable für VCO
#define MIXER_EN	    P6_8,  SCU_CONF_FUNCTION4, GPIO5, GPIOPIN16, clear

#define CLK_VCO		    P2_6,  SCU_CONF_FUNCTION4, GPIO5, GPIOPIN6,  clear
#define MOSI_VCO	    P6_4,  SCU_CONF_FUNCTION0, GPIO3, GPIOPIN3,  clear
#define LE_VCO		    P5_5,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN14, clear

#define MUX_VCO		    PB_5,  SCU_GPIO_PUP|SCU_CONF_FUNCTION4, GPIO5, GPIOPIN25 // input
#define RXENABLE		P4_5,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN5,  clear
#define TXENABLE	    P4_4,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN4,  clear

#define RXHP		    P8_1,  SCU_CONF_FUNCTION0, GPIO4, GPIOPIN1,  clear
#define CS_XCVR		    P1_20, SCU_CONF_FUNCTION0, GPIO0, GPIOPIN15, clear
#define XCVR_EN		    P4_6,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN6,  clear

#define CS_AD		    P5_7,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN7,  set     // chip select 

#define SPIFI_SCK		P3_3,  SCU_GPIO_FAST|SCU_CONF_FUNCTION3
#define SPIFI_SIO3		P3_4,  SCU_GPIO_FAST|SCU_CONF_FUNCTION3
#define SPIFI_SIO2		P3_5,  SCU_GPIO_FAST|SCU_CONF_FUNCTION3
#define SPIFI_MISO		P3_6,  SCU_GPIO_FAST|SCU_CONF_FUNCTION3
#define SPIFI_MOSI		P3_7,  SCU_GPIO_FAST|SCU_CONF_FUNCTION3
#define SPIFI_CS		P3_8,  SCU_GPIO_FAST|SCU_CONF_FUNCTION3

#define UART0_TXD       P2_0,  SCU_CONF_FUNCTION1|SCU_UART_RX_TX
#define UART0_RXD       P2_1,  SCU_CONF_FUNCTION1|SCU_UART_RX_TX

#endif /* _PIN_H */
