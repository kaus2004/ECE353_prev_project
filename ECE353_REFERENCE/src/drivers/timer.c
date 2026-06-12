/**
 * @file timer.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2024-08-14
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "timer.h"
#include <complex.h>
#include "cyhal_timer.h"
#include "buttons.h"
#include "cyhal.h" 

cy_rslt_t timer_init(cyhal_timer_t *timer_obj, cyhal_timer_cfg_t *timer_cfg, uint32_t ticks, void *Handler)
{
    cy_rslt_t rslt;
    timer_cfg->period = ticks; // Set the timer period to the specified number of ticks
    timer_cfg->direction = CYHAL_TIMER_DIR_UP; // Set the timer to count up
    timer_cfg->is_compare = false; // Disable compare mode
    //timer_cfg->compare_value = 0; // No compare value for this configuration
    timer_cfg->is_continuous = true; // Set the timer to be continuous
    timer_cfg->value = 0; // Initialize the timer value to 0

    rslt= cyhal_timer_init(timer_obj, NC, NULL); // Initialize the timer object
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // Return if initialization fails
    }
    rslt=cyhal_timer_configure(timer_obj, timer_cfg);
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // Return if configuration fails
    }

    rslt=cyhal_timer_set_frequency(timer_obj, 100000000); // Set the timer frequency to 100 MHz
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // Return if setting frequency fails
    }
    // we need to register a callback
    cyhal_timer_register_callback(timer_obj, Handler, NULL); // Register the callback handler
    
    cyhal_timer_enable_event(timer_obj, CYHAL_TIMER_IRQ_TERMINAL_COUNT, 3, true); // Enable terminal count interrupt with priority 3// mid level priority
    
    
    rslt=cyhal_timer_start(timer_obj); // Start the timer
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // Return if starting the timer fails
    }

    return rslt; // Return the result of the initialization
}