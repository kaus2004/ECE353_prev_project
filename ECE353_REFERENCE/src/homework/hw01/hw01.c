/**
 * @file hw01.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "hw01.h"

#if defined(HW01)

char APP_DESCRIPTION[] = "ECE353 F25 HW01 -- Alarm Clock";

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
hw01_state_t current_state;
// alarm_clock_info_t clock_info;
alarm_clock_info_t clock_info ;
//cyhal_timer_t alarm_timer; 
// volatile int hours;
// volatile int minutes;
alarm_en_disable_t alarm_enabled = DISABLE_ALARM;
buzzer_en_disable_t buzzer_enabled = DISABLE_BUZZER;

cyhal_timer_t buzzer_obj;
cyhal_timer_cfg_t buzzer_cfg;

//time counter for the the whole set up
static int alarm_seconds_elapsed = 0;



/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/*************************************************
* Handler used to time various events. 
*************************************************/
void handler_alarm_timer(void *arg, cyhal_timer_event_t event)
{
    if(event & CYHAL_TIMER_IRQ_TERMINAL_COUNT)
    {
        //set the interrupt flag, every 100 ms flag, 500ms flag, 5 second flag
        // Increment time by 1 minute every time this handler is called

        //use global clock_info to update time
        clock_info.time_minutes++;
        if (clock_info.time_minutes >= 60) {
            clock_info.time_minutes = 0;
            clock_info.time_hours++;
            if (clock_info.time_hours >= 24) {
                clock_info.time_hours = 0;
            }
        }
    }
    lcd_draw_time(clock_info.time_hours, clock_info.time_minutes);

}

/*************************************************
 * @brief 
 * 
 * @param alarm_info 
 * @param events 
 ************************************************/
void hw01_state_set_time(
    alarm_clock_info_t *alarm_info,
    volatile ece353_events_t *events
)
{
    //drawing the speaker in grey on LCD screen
    draw_speaker(0xC618);
    //drawing the alarm clock in yellow on LCD screen while blinking on and off
    draw_alarm_clock(0xFFE0);
    cyhal_system_delay_ms(500);
    erase_alarm_clock();
    cyhal_system_delay_ms(500);
    //display current time in white font on LCD screen
    lcd_draw_time(alarm_info->time_hours,alarm_info->time_minutes);

    //sw1 increments time hours
    if(events->sw1==1){
        events->sw1=0;
        alarm_info->time_hours=(alarm_info->time_hours+1)%24; //wrap around after 23 to 0
        lcd_draw_time(alarm_info->time_hours,alarm_info->time_minutes);
    }
    //sw2 increments time minutes
    if(events->sw2==1){
        events->sw2=0;
        alarm_info->time_minutes=(alarm_info->time_minutes+1)%60; //wrap around after 59 to 0
        lcd_draw_time(alarm_info->time_hours,alarm_info->time_minutes);
    }
}

/*************************************************
 * @brief 
 * 
 * @param alarm_info 
 * @param events 
 ************************************************/
void hw01_state_set_alarm(
    alarm_clock_info_t *alarm_info,
    volatile ece353_events_t *events
)
{
    // draw alarm clock in green
    draw_alarm_clock(0x07E0);

    //drawing the speaker in yellow on LCD screen
    draw_speaker(0xFFE0);
    //erasing speaker in yellow
    cyhal_system_delay_ms(500);
    erase_speaker();
    cyhal_system_delay_ms(500);

    //display current time in white font on LCD screen
    lcd_draw_time(alarm_info->alarm_hours,alarm_info->alarm_minutes);

    //sw1 used to increment alarm hours
    if(events->sw1==1){
        events->sw1=0;
        alarm_info->alarm_hours=(alarm_info->alarm_hours+1)%24; //wrap around after 23 to 0
        lcd_draw_time(alarm_info->alarm_hours,alarm_info->alarm_minutes);
    }
    //sw2 used to increment alarm minutes
    if(events->sw2==1){
        events->sw2=0;
        alarm_info->alarm_minutes=(alarm_info->alarm_minutes+1)%60; //wrap around after 59 to 0
        lcd_draw_time(alarm_info->alarm_hours,alarm_info->alarm_minutes);
    }
    if(events->sw3==1){
        events->sw3=0;
        current_state=STATE_HW01_RUNNING;
    }



}

/*************************************************
 * @brief 
 * 
 * @param alarm_info 
 * @param events 
 ************************************************/
void hw01_state_running(
    alarm_clock_info_t *alarm_info,
    volatile ece353_events_t *events
)
{
    //green alarm clock
    draw_alarm_clock(0x07E0);
    
    cyhal_system_delay_ms(100);
    //display current time in white font on LCD screen
    lcd_draw_time(alarm_info->time_hours,alarm_info->time_minutes);
    //sw1 enables/disables alarm
    if(alarm_enabled==DISABLE_ALARM){
            //erase_speaker() grey;
            draw_speaker(0xC618);
            //Alarm disabled

    }else{
            //erase_speaker() green;
            draw_speaker(0x07E0);
            //printf("Alarm Enabled\n");
    }
    if(events->sw1==1){
        events->sw1=0;
        //toggle alarm on/off
        if(alarm_enabled==DISABLE_ALARM){
            alarm_enabled=ENABLE_ALARM;
            //enable alarm

        }else{
            alarm_enabled=DISABLE_ALARM;
            //Alarm disabled
        }
    }

    if(alarm_enabled==ENABLE_ALARM){
        //check if current time matches alarm time
        if(alarm_info->time_hours==alarm_info->alarm_hours && alarm_info->time_minutes==alarm_info->alarm_minutes){
            current_state=STATE_HW01_ALARM_TRIGGERED;
        }
    }

    //sw3 shall transition back to set time state
    if(events->sw3){
        events->sw3=0;
        current_state=STATE_HW01_SET_TIME;
    }
    //updating timer every 100ms with 10x speed of actual time
    handler_alarm_timer(NULL,CYHAL_TIMER_IRQ_TERMINAL_COUNT);
}

/*************************************************
 * @brief 
 * 
 * @param alarm_info 
 * @param events 
 ************************************************/
void hw01_state_alarm_triggered(
    alarm_clock_info_t *alarm_info,
    volatile ece353_events_t *events
)
{
    //preventing effect of sw1 and sw3
    events->sw1=0;
    events->sw3=0;


    // cyhal_system_delay_ms(500);
    //display green alarm clock
    draw_alarm_clock(0x07E0);
    
    cyhal_system_delay_ms(500);
    //display red speaker
    draw_speaker(0xF800);

    cyhal_system_delay_ms(500);

    //display current time in white font on LCD screen
    lcd_draw_time(alarm_info->time_hours,alarm_info->time_minutes);

    //buzzer is enabled entering this state
    buzzer_enabled=ENABLE_BUZZER;
    cyhal_timer_configure(&buzzer_obj, &buzzer_cfg);    
    cyhal_timer_start(&buzzer_obj);

    //buzzer disabled after 5 seconds or sw2 pressed

    //turning alarm off after 5 seconds
    if(alarm_info->alarm_hours==alarm_info->time_hours && alarm_info->alarm_minutes==alarm_info->time_minutes+10){
        buzzer_enabled=DISABLE_BUZZER;
        current_state=STATE_HW01_RUNNING;
        alarm_seconds_elapsed=0; //resetting the 5 second counter
        cyhal_timer_stop(&buzzer_obj);
    }

    //disable buzzer via sw2
    if(ECE353_Events.sw2==1){//add 5 second limit later
        ECE353_Events.sw2=0;
        buzzer_enabled=DISABLE_BUZZER;
        current_state=STATE_HW01_RUNNING;
        alarm_seconds_elapsed=0; //resetting the 5 second counter
        cyhal_timer_stop(&buzzer_obj);
    }
    handler_alarm_timer(NULL,CYHAL_TIMER_IRQ_TERMINAL_COUNT);
    


}

/*************************************************
 * @brief 
 * 
 * @param alarm_info 
 * @param events 
 ************************************************/
void hw01_state_error(
    alarm_clock_info_t *alarm_info,
    volatile ece353_events_t *events
)
{
    printf("State: Error \r\n");

    for(int i = 0; i < 240000; i++); // Delay using Magic number for 5
    CY_ASSERT(0);

    /* Will never reach this line*/
}

void buzzer_handler(void *arg, cyhal_timer_event_t event)
{
    //OG code
    // // toggle the buzzer pin
    // if(buzzer_enabled == ENABLE_BUZZER)
    //    PORT_BUZZER->OUT_INV = MASK_BUZZER;
    static int tick_count = 0;

    if(buzzer_enabled == ENABLE_BUZZER)
    {
        PORT_BUZZER->OUT_INV = MASK_BUZZER;
        tick_count++;
        printf("%d: Buzzer on\r\n", tick_count);
        if (tick_count >= 2000) {  // 50 toggles = 5 seconds (depends on period) // Delay using Magic number for 5
            printf("Buzzer off after 5 seconds\r\n");
            buzzer_enabled = DISABLE_BUZZER;
            cyhal_timer_stop(&buzzer_obj);
            tick_count = 0;
            current_state=STATE_HW01_RUNNING;
        }
        //return 0;
    }
}

/*************************************************
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 ************************************************/
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

    rslt=buttons_init_gpio();
    // rslt=leds_init_gpio();
    //  if(rslt != CY_RSLT_SUCCESS)
    // {
    //   printf("LEDs Initialization failed with error: %d\n", rslt);
    //   CY_ASSERT(0); //halts the processor for us
    // }
    //buzzer configuration
    // buzzer_cfg.period = 50000;

    // //Added for the buzzer
    // timer_init(&buzzer_obj, &buzzer_cfg, 100, PORT_BUZZER->OUT_INV = MASK_BUZZER);// Initialize the timer with 0 ticks to start
    // cyhal_gpio_init(PIN_BUZZER, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0); // Initialize the buzzer pin as an output and set it low (off)

    buzzer_cfg.period = 50000;  // 50k ticks
    timer_init(&buzzer_obj, &buzzer_cfg, buzzer_cfg.period, buzzer_handler);
    cyhal_gpio_init(PIN_BUZZER, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);


    rslt=buttons_init_timer();
    rslt =lcd_initialize();
    if(rslt != CY_RSLT_SUCCESS)
    {
      printf("LCD Initialization failed with error: %d\n", rslt);
      CY_ASSERT(0); //halts the processor for us
    }
    // //initializing the buttons
    //cyhal_system_delay_ms(100); 
    lcd_clear_screen(LCD_COLOR_BLACK);
    lcd_draw_time(clock_info.time_hours=0,clock_info.time_minutes=0);
    
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
    current_state=STATE_HW01_INIT;
    while(1)
    {
        switch(current_state){
            case STATE_HW01_INIT:
                //initialize the alarm_info struct
                clock_info.time_hours=0;
                clock_info.time_minutes=0;
                lcd_draw_time(clock_info.time_hours,clock_info.time_minutes);
                //initialize the timer
                //cyhal_system_delay_ms(100); // give 100 ms for LCD to show
                current_state=STATE_HW01_SET_TIME;
                break;
            case STATE_HW01_SET_TIME:
                hw01_state_set_time(&clock_info, &ECE353_Events);
                if(ECE353_Events.sw3==1){
                   ECE353_Events.sw3=0;
                   current_state=STATE_HW01_SET_ALARM;
                };
                break;
            case STATE_HW01_SET_ALARM:
                hw01_state_set_alarm(&clock_info, &ECE353_Events);
                break;
            case STATE_HW01_RUNNING:
                // if(alarm_enabled!=last_alarm_enabled){
                hw01_state_running(&clock_info, &ECE353_Events);
                break;
            case STATE_HW01_ALARM_TRIGGERED:
                hw01_state_alarm_triggered(&clock_info, &ECE353_Events);
                break;
            case STATE_HW01_ERROR:
                hw01_state_error(&clock_info, &ECE353_Events);
                break;
            default:
                current_state=STATE_HW01_ERROR;
                //should never be reached
                break;
        }
    }
}
#endif