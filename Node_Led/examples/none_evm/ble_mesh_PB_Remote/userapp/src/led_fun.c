#include <stdint.h>
#include "string.h"

#include "driver_gpio.h"
#include "driver_uart.h"
//#include "driver_plf.h"
#include "driver_system.h"
//#include "driver_ssp.h"
#include "driver_pmu.h"
//#include "driver_efuse.h"
//#include "driver_frspim.h"
#include "driver_flash.h"
#include "driver_pwm.h"
#include "led_fun.h"
#include "co_printf.h"

LED_CTR Led_Ctr;

void led_pwm_init(void)
{
	#ifdef REVER_FLAG
	Led_Ctr.blue_duty = Led_Ctr.green_duty = Led_Ctr.red_duty  = PWM_MAX_DUTY;
	#else
	Led_Ctr.blue_duty = Led_Ctr.green_duty = Led_Ctr.red_duty  = 00;
	#endif
	
	system_set_port_mux(LED_PWM_PORT, RED_PIN, RED_PIN_PWM);
	system_set_port_mux(LED_PWM_PORT, GREEN_PIN, GREEN_PIN_PWM);
	system_set_port_mux(LED_PWM_PORT, BLUE_PIN, BLUE_PIN_PWM);
	
	#if 0
	pmu_set_port_mux(LED_PWM_PORT, RED_PIN, PMU_PORT_MUX_PWM);
	pmu_set_port_mux(LED_PWM_PORT, GREEN_PIN, PMU_PORT_MUX_PWM);
	pmu_set_port_mux(LED_PWM_PORT, BLUE_PIN, PMU_PORT_MUX_PWM);
	#endif
	
	pwm_init(RED_PWN_CHN, PWM_FREQUENCE, Led_Ctr.red_duty);
	pwm_start(RED_PWN_CHN);
	pwm_init(GREEN_PWN_CHN, PWM_FREQUENCE, Led_Ctr.green_duty);
	pwm_start(GREEN_PWN_CHN);
	
	pwm_init(BLUE_PWN_CHN, PWM_FREQUENCE, Led_Ctr.blue_duty);
	pwm_start(BLUE_PWN_CHN);	
}

void update_led_pwm(uint8_t r, uint8_t g, uint8_t b)
{	
	co_printf("r1[%d] g1[%d] b1[%d]\n", r, g, b);
	Led_Ctr.red_duty = r * DUTY_RATE;
	Led_Ctr.green_duty = g * DUTY_RATE;
	Led_Ctr.blue_duty = b * DUTY_RATE;
	co_printf("r2[%d] g2[%d] b2[%d]\n", Led_Ctr.red_duty, Led_Ctr.green_duty, Led_Ctr.blue_duty);
	#ifdef REVER_FLAG
	Led_Ctr.red_duty = PWM_MAX_DUTY - Led_Ctr.red_duty;
	Led_Ctr.green_duty = PWM_MAX_DUTY - Led_Ctr.green_duty;
	Led_Ctr.blue_duty = PWM_MAX_DUTY - Led_Ctr.blue_duty;
	co_printf("r3[%d] g3[%d] b3[%d]\n", Led_Ctr.red_duty, Led_Ctr.green_duty, Led_Ctr.blue_duty);
	#endif
	
	pwm_update(RED_PWN_CHN, PWM_FREQUENCE, Led_Ctr.red_duty);
	pwm_update(GREEN_PWN_CHN, PWM_FREQUENCE, Led_Ctr.green_duty);
	pwm_update(BLUE_PWN_CHN, PWM_FREQUENCE, Led_Ctr.blue_duty);
}

void color_temperature_set(uint16_t tcolar)
{
	uint16_t value = tcolar/100;
	
	#if 1
		if(value < (TCOLAR_MAX_VAL>>1)) {
			Led_Ctr.red_duty = (value-TCOLAR_MIN_VAL) * COLOR_TEMPERA_RATE;
			if(Led_Ctr.red_duty < 2)
				Led_Ctr.red_duty = 2;
			Led_Ctr.green_duty = Led_Ctr.red_duty >> 2;
			if(!Led_Ctr.green_duty) 
				Led_Ctr.green_duty = 1;
			Led_Ctr.blue_duty = Led_Ctr.green_duty * 0.7;
			if(!Led_Ctr.blue_duty)	
				Led_Ctr.blue_duty = 1;
			co_printf("@-->Nuan:[%d] [%d] [%d]\n", Led_Ctr.red_duty, Led_Ctr.green_duty, Led_Ctr.blue_duty);
		} else {
			Led_Ctr.red_duty = (value-TCOLAR_MIN_VAL) * COLOR_TEMPERA_RATE;
			Led_Ctr.blue_duty = Led_Ctr.red_duty; 
			Led_Ctr.green_duty = Led_Ctr.red_duty;
			co_printf("@-->Leng:[%d] [%d] [%d]\n", Led_Ctr.red_duty, Led_Ctr.green_duty, Led_Ctr.blue_duty);
		}
	#else
		Led_Ctr.red_duty = PWM_MAX_DUTY;
		Led_Ctr.green_duty = (PWM_MAX_DUTY-tcolar*55/TCOLAR_MAX_VAL);
		Led_Ctr.blue_duty = (PWM_MAX_DUTY-tcolar*PWM_MAX_DUTY/TCOLAR_MAX_VAL);
	#endif
		
	#ifdef REVER_FLAG
	Led_Ctr.red_duty = PWM_MAX_DUTY - Led_Ctr.red_duty;
	Led_Ctr.green_duty = PWM_MAX_DUTY - Led_Ctr.green_duty;
	Led_Ctr.blue_duty = PWM_MAX_DUTY - Led_Ctr.blue_duty;
	co_printf("rr[%d] gg[%d] bb[%d]\n", Led_Ctr.red_duty, Led_Ctr.green_duty, Led_Ctr.blue_duty);
	#endif
	
	pwm_update(RED_PWN_CHN, PWM_FREQUENCE, Led_Ctr.red_duty);
	pwm_update(GREEN_PWN_CHN, PWM_FREQUENCE, Led_Ctr.green_duty);
	pwm_update(BLUE_PWN_CHN, PWM_FREQUENCE, Led_Ctr.blue_duty);
}
