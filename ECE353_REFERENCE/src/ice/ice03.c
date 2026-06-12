/**
 * @file ice02.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE03)
#include "drivers.h"
#include <stdio.h>

char APP_DESCRIPTION[] = "ECE353: ICE 03 - Timer Interrupts/Debounce Buttons";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

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

    //enabling the pins connected to RGB LED as outputs
    //initialize pins
    // Initialize RGB LED pins as outputs
    // cyhal_gpio_init(P9_0, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1); // Red OFF
    // cyhal_gpio_init(P9_2, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0); // Green OFF
    // cyhal_gpio_init(P8_0, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0); // Blue OFF
    
    // // Initialize the buttons
    // //enable the pins connected to SW1, SW2, and SW3 as inputs

    // rslt = buttons_init_gpio();
    // if(rslt != CY_RSLT_SUCCESS)
    // {
    //     printf("Failed to initialize buttons\n");
    //     CY_ASSERT(0);
    // }
    // // Initialize the timer for button debouncing
    // rslt = buttons_init_timer();
    // if(rslt != CY_RSLT_SUCCESS)
    // {
    //     printf("Failed to initialize button timer\n");
    //     CY_ASSERT(0);
    // }

    //Initialize the buttons using buttons_init_gpio
    buttons_init_gpio();
    leds_init_gpio();
    buttons_init_timer();
    //




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

    state03_t state = INIT;
    //cyhal delay
    while(1){  
        // Initialize RGB LED pins as outputs
        
            switch(state){

               case INIT:
                     cyhal_gpio_write(P9_0, 1); // Turn on the Red LED
                     cyhal_gpio_write(P9_2, 0);
                     cyhal_gpio_write(P8_0, 0);
                    {
                        if(ECE353_Events.sw1==1){
                            ECE353_Events.sw1=0;
                            state=SW1_DET;
                            printf("SW1 Pressed\n");
                            
                        }
                        if(ECE353_Events.sw2==1 || ECE353_Events.sw3==1){
                            ECE353_Events.sw2=0;
                            ECE353_Events.sw3=0;
                            state=INIT;
                        }

                    }
                    break;
                case SW1_DET:
                    //switch on red LED
                     cyhal_gpio_write(P9_0, 1); // Turn on the Red LED
                     cyhal_gpio_write(P9_2, 0);
                     cyhal_gpio_write(P8_0, 1); //blue on 
                    {
                        if(ECE353_Events.sw2==1){
                            ECE353_Events.sw2=0;
                            state=SW2_DET_1;

                    
                        }else if(ECE353_Events.sw1==1 || ECE353_Events.sw3==1){
                            ECE353_Events.sw1=0;
                            ECE353_Events.sw3=0;
                            state=INIT;
                            
                        }
                    }
                    break;
                case SW2_DET_1:
                    //switch off red LED
                    cyhal_gpio_write(P9_0, 0); // Turn off the Red LED
                    cyhal_gpio_write(P8_0, 1); // Turn off the Blue LED
                    cyhal_gpio_write(P9_2, 0); // Turn off the Green LED
                    {
                        if(ECE353_Events.sw2==1){
                            ECE353_Events.sw2=0;
                            state=SW2_DET_2;
                            
                            ECE353_Events.sw2=0;
                        }else if(ECE353_Events.sw1==1 || ECE353_Events.sw3==1){
                            ECE353_Events.sw1=0;
                            ECE353_Events.sw3=0;
                            state=INIT;
            
                        }
                    }
                    break;
                case SW2_DET_2:
                    //switch off red LED
                    cyhal_gpio_write(P9_0, 0); // Turn off the Red LED
                    cyhal_gpio_write(P8_0, 1); // Turn on the Blue LED
                    cyhal_gpio_write(P9_2, 1); // Turn on the Green LED
                    {
                        if(ECE353_Events.sw3==1){
                            ECE353_Events.sw3=0;
                            state=SW3_DET;
                           
                            ECE353_Events.sw3=0;
                        }else if(ECE353_Events.sw1==1 || ECE353_Events.sw2==1){
                            ECE353_Events.sw1=0;
                            ECE353_Events.sw2=0;
                            state=INIT;
                            
                        }
                    }
                    break;
                case SW3_DET:
                    //switch off red LED //we never use gpio main ICE// LEDs weren't defined 
                    cyhal_gpio_write(P9_0, 0); // Turn off the Red LED
                    cyhal_gpio_write(P8_0, 0); // Turn off the Blue LED
                    cyhal_gpio_write(P9_2, 1); // Turn on the Green LED
                    {//this is a steady state so nothing happens once reached
                    //nothing happens
                    }
                    break; 
                default:
                    state=INIT;
                    break;
            }
        
    }
}
#endif
