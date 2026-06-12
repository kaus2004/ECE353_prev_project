/**
 * @file ice01.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE01)
#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: ICE 01 - Memory Mapped IO - GPIO";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
button_state_t prev_state=BUTTON_STATE_LOW;
ece353_button_t button_pressed;

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
    console_init();
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

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


  //initializing the buttons
    while (1) {
        // Check SW1
        button_state_t state1 = buttons_get_state(BUTTON_SW1);
        if (state1 == BUTTON_STATE_FALLING_EDGE) {
            printf("SW1 pressed\n");
            //turn on red led using leds_set_state
            leds_set_state(LED_RED, LED_STATE_ON);
        } else if (state1 == BUTTON_STATE_RISING_EDGE) {
            printf("SW1 released\n");
            leds_set_state(LED_RED, LED_STATE_OFF);
        }

        //check sw2
        button_state_t state2 = buttons_get_state(BUTTON_SW2);
        if (state2 == BUTTON_STATE_FALLING_EDGE) {
            printf("SW2 pressed\n");
            leds_set_state(LED_GREEN, LED_STATE_ON);

        } else if (state2 == BUTTON_STATE_RISING_EDGE) {
            printf("SW2 released\n");
            leds_set_state(LED_GREEN, LED_STATE_OFF);
            
        }

        // Check SW3
        button_state_t state3 = buttons_get_state(BUTTON_SW3);
        if (state3 == BUTTON_STATE_FALLING_EDGE) {
            printf("SW3 pressed\n");
            leds_set_state(LED_BLUE, LED_STATE_ON);

        } else if (state3 == BUTTON_STATE_RISING_EDGE) {
            printf("SW3 released\n");
            leds_set_state(LED_BLUE, LED_STATE_OFF);
        }

        cyhal_system_delay_ms(100);

    }

// Initialize board and console


}
#endif
