/**
 * @file ex01.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(EX01)
#include "drivers.h"
//new stuff to test buttons



char APP_DESCRIPTION[] = "ECE353: Example 01 - Intro to C";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
 // Keep track of previous SW1 state
button_state_t prev_state = BUTTON_STATE_LOW;

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
    cy_rslt_t rslt; // This is a return code
    console_init();
    
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    /*Initialize P9.2 as an output*/
    rslt=cyhal_gpio_init(PIN_LED_GREEN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    if(rslt != CY_RSLT_SUCCESS){
        printf("Failed to initialize User LED\n\r");
        CY_ASSERT(0);

    }
    //initializing the buttons
    buttons_init_gpio();
    leds_init_gpio();

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
    /* Enter Infinite Loop*/
    while (1)
    {
        // cyhal_gpio_init(PIN_LED_RED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
        // //use cyhal to read if button is pressed
        // if(cyhal_gpio_read(PIN_BUTTON_SW3)==0){
        //     printf("SW1 pressed\n");
        //     leds_set_state(LED_RED, LED_STATE_ON);
        // }
        // else{
        //    printf("Hello World!\n");
        // }
        // cyhal_system_delay_ms(1000);
        
        //turn on green led without clicking
        
        leds_set_state(LED_GREEN, LED_STATE_ON);
        cyhal_system_delay_ms(1000);
        leds_set_state(LED_GREEN, LED_STATE_OFF);
        cyhal_system_delay_ms(1000);

       

    }
}
#endif