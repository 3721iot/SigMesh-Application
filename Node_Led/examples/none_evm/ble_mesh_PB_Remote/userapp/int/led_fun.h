#ifndef _LED_FUN_H
#define _LED_FUN_H

#include <stdint.h>

#define LED_PWM_PORT 		GPIO_PORT_A

#define RED_PIN				GPIO_BIT_7
#define GREEN_PIN			GPIO_BIT_4
#define BLUE_PIN			GPIO_BIT_5

//#define RED_PWN_CHN		PWM_CHANNEL_1 		//PWM1  PA1
#define RED_PWN_CHN			PWM_CHANNEL_1 		//PWM1  PA7
#define GREEN_PWN_CHN		PWM_CHANNEL_4		//PWM2	PA4
#define BLUE_PWN_CHN		PWM_CHANNEL_5		//PWM3	PA5

#define RED_PIN_PWM			PORTA7_FUNC_PWM1	//pin resuse
#define GREEN_PIN_PWM		PORTA4_FUNC_PWM4			
#define BLUE_PIN_PWM		PORTA5_FUNC_PWM5	//

#define PWM_FREQUENCE		10000
 #define DUTY_RATE			0.39
#define PWM_MAX_DUTY		99					//pwm max valtage
#define TCOLAR_MAX_VAL		200
#define TCOLAR_MIN_VAL		8 
#define TCOLAR_ADJUST		5
#define COLOR_TEMPERA_RATE	0.52


//#define REVER_FLAG	//Ctrol Led Lever Need Revert	

typedef struct{
	uint8_t red_duty;
	uint8_t green_duty;
	uint8_t blue_duty;
}LED_CTR;

extern LED_CTR Led_Ctr;

extern void led_pwm_init(void);
extern void update_led_pwm(uint8_t r, uint8_t g, uint8_t b);
extern void color_temperature_set(uint16_t tcolar);

#endif
