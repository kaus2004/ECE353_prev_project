/**
 * @file ex04.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(EX04)
#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: Example 04 - PWM with Interrupts";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/
// The general formula for calculating the number of ticks for a given frequency is:
// Ticks = (1 / Desired Frequency) / (1 / Timer Frequency) = Tick Count
//
// If we re-arrange the formula, we can see that:
// Tick Count = Timer Frequency / Desired Frequency
//
// For a 1000 Hz frequency with a 100 MHz timer, the calculation is:
// Tick Count = 100000000 / 1000 = 100000 ticks

// Because we need to toggle the buzzer on and off, we need to set the timer to 
// half the frequency, so we divide the tick count by 2.
#define TIMER_FREQUENCY 100000000       // Timer frequency in Hz
#define BUZZER_FREQUENCY_KHZ_1000 1000  // Desired buzzer frequency in Hz
#define BUZZER_FREQUENCY_KHZ_1500 1500  // Desired buzzer frequency in Hz
#define BUZZER_FREQUENCY_KHZ_2000 2000  // Desired buzzer frequency in Hz
#define BUZZER_FREQUENCY_KHZ_2500 2500  // Desired buzzer frequency in Hz
#define BUZZER_FREQUENCY_KHZ_3000 3000  // Desired buzzer frequency in Hz

#define BUZZER_TICK_COUNT_1000_KHZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_KHZ_1000) / 2 //for twice as many interrupts
#define BUZZER_TICK_COUNT_1500_KHZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_KHZ_1500) / 2
#define BUZZER_TICK_COUNT_2000_KHZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_KHZ_2000) / 2
#define BUZZER_TICK_COUNT_2500_KHZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_KHZ_2500) / 2
#define BUZZER_TICK_COUNT_3000_KHZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_KHZ_3000) / 2


/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_timer_t buzzer_timer;
cyhal_timer_cfg_t buzzer_timer_cfg;

typedef enum{
    BUZZER_INDEX_HZ_0000 = 0,
    BUZZER_INDEX_HZ_1000,
    BUZZER_INDEX_HZ_1500,
    BUZZER_INDEX_HZ_2000,
    BUZZER_INDEX_HZ_2500,
    BUZZER_INDEX_HZ_3000,
} buzzer_index_t;

uint32_t Buzzer_Tick[] = {
    0,
    BUZZER_TICK_COUNT_1000_KHZ,
    BUZZER_TICK_COUNT_1500_KHZ,
    BUZZER_TICK_COUNT_2000_KHZ,
    BUZZER_TICK_COUNT_2500_KHZ,
    BUZZER_TICK_COUNT_3000_KHZ,

};

char Buzzer_DBG_Messages[][20] = {
    "Buzzer Off",
    "Buzzer 1.0 KHz",
    "Buzzer 1.5 KHz",
    "Buzzer 2.0 KHz",
    "Buzzer 2.5 KHz",
    "Buzzer 3.0 KHz",
};

cyhal_timer_t buzzer_obj;
cyhal_timer_cfg_t buzzer_cfg;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

void buzzer_handler(void *arg, cyhal_timer_event_t event)
{
    PORT_BUZZER->OUT_INV = MASK_BUZZER; // Toggle the buzzer pin
}

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

    /* Configure the gpio pin connected to buzzer as an output pin*/
    buttons_init_gpio();
    buttons_init_timer();
    timer_init(&buzzer_obj, &buzzer_cfg, Buzzer_Tick[BUZZER_INDEX_HZ_1000], buzzer_handler);// Initialize the timer with 0 ticks to start
    cyhal_gpio_init(PIN_BUZZER, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0); // Initialize the buzzer pin as an output and set it low (off)

    



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
    buzzer_index_t buzzer_index = BUZZER_INDEX_HZ_1000;
    while (1)
    {

        if(ECE353_Events.sw1 == 1)
        {
            printf("SW1 Pressed\n\r");
            ECE353_Events.sw1 = 0;
            buzzer_index++;

            if(buzzer_index >= sizeof(Buzzer_Tick)/sizeof(Buzzer_Tick[0]))
            {
                buzzer_index = BUZZER_INDEX_HZ_0000;
            }
    
            cyhal_timer_stop(&buzzer_obj);

            //we only turn on the timer if we have a ticks count greater than 0
            if(Buzzer_Tick[buzzer_index] > 0){
                buzzer_cfg.period = Buzzer_Tick[buzzer_index];
                cyhal_timer_configure(&buzzer_obj, &buzzer_cfg);    
                cyhal_timer_start(&buzzer_obj);

            }
            printf("%s\n\r", Buzzer_DBG_Messages[buzzer_index]);
            
            
        }
    }
}
#endif
