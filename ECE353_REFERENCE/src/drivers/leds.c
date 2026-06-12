/**
 * @file leds.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "leds.h"


cy_rslt_t leds_init_gpio(void){
    cy_rslt_t rslt ; //originally = CY_RSLT_SUCCESS;

    //We initialize these pins, if anything except success returned, we return the error
    rslt =cyhal_gpio_init(PIN_LED_RED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    if(rslt != CY_RSLT_SUCCESS) return rslt;
    rslt =cyhal_gpio_init(PIN_LED_GREEN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    if(rslt != CY_RSLT_SUCCESS) return rslt;
    rslt =cyhal_gpio_init(PIN_LED_BLUE, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    if(rslt != CY_RSLT_SUCCESS) return rslt;

    return rslt;
}

void leds_set_state(ece353_led_t led, ece353_led_state_t state){
    switch(led) {
        case LED_RED:
            cyhal_gpio_write(PIN_LED_RED, (state == LED_STATE_ON) ? 1 : 0);//otriginal 0 :1//after testing yes the clicks are active high

            break;
        case LED_GREEN:
            cyhal_gpio_write(PIN_LED_GREEN, (state == LED_STATE_ON) ? 1 : 0);//otriginal 0 :1
            break;
        case LED_BLUE:
            cyhal_gpio_write(PIN_LED_BLUE, (state == LED_STATE_ON) ? 1 : 0);//otriginal 0 :1
            break;
        default:
            // Handle invalid LED selection if necessary
            break;
    }
}
 
 cy_rslt_t leds_init_pwm(cyhal_pwm_t *pwm_obj_red, cyhal_pwm_t *pwm_obj_green, cyhal_pwm_t *pwm_obj_blue)
{

    
    cy_rslt_t result;

    result = cyhal_pwm_init(pwm_obj_red, PIN_LED_RED, NULL);
    if (result != CY_RSLT_SUCCESS)
    {
        return result;
    }
    result = cyhal_pwm_init(pwm_obj_green, PIN_LED_GREEN, NULL);
    if (result != CY_RSLT_SUCCESS)
    {
        return result;
    }
    result = cyhal_pwm_init(pwm_obj_blue, PIN_LED_BLUE, NULL);
    if (result != CY_RSLT_SUCCESS)
    {
        return result;
    }

    return CY_RSLT_SUCCESS;

    //set duty cycle and frequency
    // cyhal_pwm_set_duty_cycle(pwm_obj_red, 0, 1000); // 50% duty cycle at 1 kHz
    // cyhal_pwm_set_duty_cycle(pwm_obj_green, 0, 1000); // 50% duty cycle at 1 kHz
    // cyhal_pwm_set_duty_cycle(pwm_obj_blue, 0, 1000); // 50% duty cycle at 1 kHz

    //
}