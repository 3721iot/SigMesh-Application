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
uint8_t poweron_count = 0;

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

void color_temperature_set(uint16_t tcolar, uint16_t brightness)
{
	uint16_t value = tcolar/100;
	Led_Ctr.red_duty = 99;
	Led_Ctr.green_duty = 30 + (value-TCOLAR_MIN_VAL) * COLOR_GREEN_RATE;
	Led_Ctr.blue_duty = 0;
	if(value > TCOLAR_MIDEL_VAL) {
		Led_Ctr.blue_duty = COLOR_BLUE_RATE*(value-TCOLAR_MIDEL_VAL);
		//co_printf("@-->Leng\n");
	}
	co_printf("@-->CR[%d] CG[%d] CB[%d]\n", Led_Ctr.red_duty, Led_Ctr.green_duty, Led_Ctr.blue_duty);

	value = brightness/100;
	if(value <= LIGHTNESS_MIN_VAL)
		value = LIGHTNESS_MIN_VAL + 10;
	float lrate = (float)(value-LIGHTNESS_MIN_VAL) / (float)LIGHTNESS_RANGE;
	co_printf("@-->Lrate:[%d],value:[%d]\n", (int)(lrate*100), value);
	Led_Ctr.red_duty *= lrate;
	Led_Ctr.green_duty *= lrate;
	if(!Led_Ctr.green_duty)
		Led_Ctr.green_duty = 1;
	Led_Ctr.blue_duty *= lrate;
	co_printf("@-->LR[%d] LG[%d] LB[%d]\n", Led_Ctr.red_duty, Led_Ctr.green_duty, Led_Ctr.blue_duty);
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

void Bubls_UnconfigNet_Indacate(void)
{
	static uint8_t duty = 99;
	static bool reduce_flag = true;
	
	pwm_update(RED_PWN_CHN, PWM_FREQUENCE, duty);
	pwm_update(GREEN_PWN_CHN, PWM_FREQUENCE, duty);
	pwm_update(BLUE_PWN_CHN, PWM_FREQUENCE, duty);
	if(reduce_flag) {
		duty -= 9;
		if(!duty) {
			reduce_flag = false;
			duty = 0;
		}
	} else {
		duty += 9;
		if(duty > 99) {
			duty = 99;
			reduce_flag = true;
		}
	}
}

void Bubls_ConfigNet_Ok_Indacate(void)
{
	pwm_update(RED_PWN_CHN, PWM_FREQUENCE, 99);
	pwm_update(GREEN_PWN_CHN, PWM_FREQUENCE, 99);
	pwm_update(BLUE_PWN_CHN, PWM_FREQUENCE, 99);
}