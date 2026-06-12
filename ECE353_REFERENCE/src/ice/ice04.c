/**
 * @file ice04.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE04)
#include "drivers.h"
#include <stdio.h>

char APP_DESCRIPTION[] = "ECE353: ICE 04 - PWM Buzzer";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_pwm_t red_pwm;
cyhal_pwm_t green_pwm;
cyhal_pwm_t blue_pwm;


/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");
    
    //intialize buttons
    rslt = buttons_init_gpio();
    if(rslt!= CY_RSLT_SUCCESS){
        printf("Failed to initialize buttons\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);
    }
    
    rslt=buttons_init_timer();
    if(rslt!= CY_RSLT_SUCCESS){
        printf("Failed to initialize button timer\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);
    }

    //initialize the pwm pins using cyhal_pwm_init
    rslt = leds_init_pwm(&red_pwm, &green_pwm, &blue_pwm);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize PWM for RGB LED\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);
    }
    //set the frequency of the pwm using cyhal_pwm_set_frequency

    

}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{
    while(1)
    {
        /* ADD CODE */
        //RED LED's intensity increases from 0% to 100% in 10% increments over 1 second then reset back to 0% when sw1 pressed
        if(ECE353_Events.sw1==1){
            
            cyhal_pwm_set_duty_cycle(&red_pwm, 100 , 100); // Set duty cycle (0-100%)
            cyhal_pwm_start(&red_pwm); // Start PWM
            cyhal_system_delay_ms(100); // Delay for 100 ms
            

            // cyhal_pwm_stop(&red_pwm); // Stop PWM
            cyhal_pwm_set_duty_cycle(&red_pwm, 0, 100); // Reset duty cycle to 0%
        }
        //GREEN LED's intensity increases from 0% to 100% in 10% increments over 1 second then reset back to 0% when sw2 pressed

        if(ECE353.Events.sw2==1){
            for(int duty_cycle = 0; duty_cycle <= 100; duty_cycle += 10){
                cyhal_pwm_set_duty_cycle(&green_pwm, 100, 100); // Set duty cycle (0-100%)
                cyhal_pwm_start(&green_pwm); // Start PWM
                cyhal_system_delay_ms(100); // Delay for 100 ms
            }
            cyhal_pwm_stop(&green_pwm); // Stop PWM
            cyhal_pwm_set_duty_cycle(&green_pwm, 0, 100); // Reset duty cycle to 0%
        }
        //BLUE LED's intensity increases from 0% to 100% in 10% increments over 1 second then reset back to 0% when sw3 pressed
        if(ECE353_Events.sw3==1){
            for(int duty_cycle = 0; duty_cycle <= 100; duty_cycle += 10){
                cyhal_pwm_set_duty_cycle(&blue_pwm, duty_cycle, 100); // Set duty cycle (0-100%)
                cyhal_pwm_start(&blue_pwm); // Start PWM
                cyhal_system_delay_ms(100); // Delay for 100 ms
            }
            cyhal_pwm_stop(&blue_pwm); // Stop PWM
            cyhal_pwm_set_duty_cycle(&blue_pwm, 0, 100); // Reset duty cycle to 0%
        }


        /* END ADD CODE */
    }
}
#endif
