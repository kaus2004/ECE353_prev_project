/**
 * @file task_console_rx.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#ifdef ECE353_FREERTOS
#include "drivers.h"
#include "task_console.h"
#include "cyhal_uart.h"


/**
 * @brief
 * This file contains the implementation of the console receive (Rx) task.
 * The task is responsible for processing incoming console commands and
 * controlling the state of the LEDs accordingly.
 * 
 * The task uses a double buffer to process the incoming console commands.
 * The supported commands will be "RED_ON" and "RED_OFF" to control the red LED.
 */

/* ADD CODE */
/* Global Variables */
console_buffer_t console_buffer1;
console_buffer_t console_buffer2; 

//Alllcate pointer for buffers
console_buffer_t *produce_console_buffer = NULL; //Buffer being written to by ISR
console_buffer_t *consume_console_buffer = NULL; //Buffer being read by task

//Allocate a task handle
TaskHandle_t TaskHandle_Console_Rx = NULL; //we are just allocationg a pointer not an array

// Queue used to receive device responses when commands request data
extern QueueHandle_t xQueue_Console_Rx_Responses = NULL;

//Local Function to clear buffers
void clear_consume_console_buffers(void)
{
    consume_console_buffer->index = 0;
    memset(consume_console_buffer->data, 0, CONSOLE_MAX_MESSAGE_LENGTH);
}

/**
 * @brief
 * This function is the bottom half task for receiving console input.
 *
 * It waits for a task notification from the ISR indicating that a new 
 * command has been received. The task then processes the command and 
 * controls the state of the LEDs accordingly.
 * Prior ICE 09 code in while loop
 *      //Process the data pointed by console buffer pointer
        // if(strcmp(consume_console_buffer->data, "RED_ON\n") == 0)
        // {
        //     //If red on, turn on the red led
        //     printf("Turning on RED LED\n");
        //     cyhal_gpio_write(PIN_LED_RED, CYBSP_LED_STATE_OFF);
        // }
        // else if(strcmp(consume_console_buffer->data, "RED_OFF\n") == 0)
        // {
        //     //if REd_OFF, turn off the red led
        //     cyhal_gpio_write(PIN_LED_RED, CYBSP_LED_STATE_ON);
        // }

 * MODIFY for HW05
 * @param param Unused parameter
 */
void task_console_rx(void *param)
{
    (void)param; // Unused parameter
    while (1)
    {
        // Wait for a task notification from the ISR
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
         vTaskDelay(250);
        /* Ensure the buffer is null terminated (should already be from ISR) */
        consume_console_buffer->data[consume_console_buffer->index] = '\0';

        /* Convert input to lower-case to allow case-insensitive commands */
        for (int i = 0; consume_console_buffer->data[i] != '\0'; ++i)
        {
            consume_console_buffer->data[i] = (char) tolower((int) consume_console_buffer->data[i]);
        }

        /* Tokenize the command: <command> [subcommand] [address] [value]
         * address/value are interpreted as hex when provided (e.g. 0x1A)
         */
        char *command = strtok(consume_console_buffer->data, " ");
        char *subcommand = strtok(NULL, " ");
        char *address = strtok(NULL, " ");
        uint16_t address_int = (address != NULL) ? (uint16_t) strtol(address, NULL, 16) : 0xEEEE;
        char *value = strtok(NULL, " ");
        uint8_t value_int = (value != NULL) ? (uint8_t) strtol(value, NULL, 16) : 0xEE;

        if (command == NULL)
        {
            continue;
        }
        // device_request_msg_t request_packet;
        // xQueueSend(Queue_IMU_Requests, &request_packet, portMAX_DELAY);

        if (strncmp(command, "red_on", 7) == 0 || strncmp(command, "red_on\r", 7) == 0)
        {
            cyhal_gpio_write(PIN_LED_RED, 1);
            task_console_printf("Red LED is ON\r\n");
        }
        else if (strncmp(command, "red_off", 8) == 0 || strncmp(command, "red_off\r", 8) == 0)
        {
            cyhal_gpio_write(PIN_LED_RED, 0);
            task_console_printf("Red LED is OFF\r\n");
        }
        else if (strncmp(command, "imu", 4) == 0) {
            //task_console_printf("Requesting IMU Data...\r\n");
            device_request_msg_t request_packet;
            device_response_msg_t response_packet;

            request_packet.device = DEVICE_IMU;
            request_packet.operation = DEVICE_OP_READ;
            request_packet.response_queue = xQueue_Console_Rx_Responses;

            //task_console_printf("Sending request to IMU task...\r\n");

            // Send the request to the IMU task
            xQueueSend(Queue_IMU_Requests, &request_packet, portMAX_DELAY);

            //task_console_printf("Waiting for response from IMU task...\r\n");

            vTaskDelay(pdMS_TO_TICKS(250));
            // Wait for the response from the IMU task: Queue was empty
            if(xQueueReceive(xQueue_Console_Rx_Responses, &response_packet,
                        pdMS_TO_TICKS(1000)) == pdPASS)
            {
               // Check the status of the response
               if (response_packet.status == DEVICE_OPERATION_STATUS_READ_SUCCESS) {
                    task_console_printf(
                      "IMU Data: X=%d, Y=%d, Z=%d\r\n", response_packet.payload.imu[0],
                                response_packet.payload.imu[1], response_packet.payload.imu[2]);
                } 
            }else {
                task_console_printf("ERROR: IMU Read Failed\r\n");
            }
        } 
        else if (strncmp(command, "light", 6) == 0)
        {
            uint16_t ambient_light = 0;
            if (system_sensors_get_light(xQueue_Console_Rx_Responses, &ambient_light))
            {
                task_console_printf("Ambient Light: %u\r\n", ambient_light);
            }
            else
            {
                task_console_printf("ERROR: Light Sensor Read Failed\r\n");
            }
        }
        else if (strncmp(command, "ioexp", 6) == 0)
        {
            if (subcommand == NULL)
            {
                task_console_printf("ERROR: IOEXP command requires a subcommand\r\n");
                continue;
            }
            if (strncmp(subcommand, "r", 2) == 0)
            {
                uint8_t read_value = 0;
                if (system_sensors_io_expander_read(xQueue_Console_Rx_Responses, (uint8_t) address_int, &read_value))
                {
                    task_console_printf("IOEXP Read Success: Addr=0x%02X Val=0x%02X\r\n", (uint8_t) address_int, read_value);
                }
                else
                {
                    task_console_printf("ERROR: IOEXP Read Failed at Addr=0x%02X\r\n", (uint8_t) address_int);
                }
            }
            else if (strncmp(subcommand, "w", 2) == 0)
            {
                if (system_sensors_io_expander_write(xQueue_Console_Rx_Responses, (uint8_t) address_int, value_int))
                {
                    //create buffer
                    device_response_msg_t response;
                    xQueueReceive(xQueue_Console_Rx_Responses, &response, portMAX_DELAY); // Clear response
                    task_console_printf("IOEXP Write Success: Addr=0x%02X Val=0x%02X\r\n", (uint8_t) address_int, value_int);
                }
                else
                {
                    task_console_printf("ERROR: IOEXP Write Failed at Addr=0x%02X\r\n", (uint8_t) address_int);
                }
            }
            else
            {
                task_console_printf("ERROR: Unknown IOEXP subcommand: %s\r\n", subcommand);
            }
        }
        else if (strncmp(command, "eeprom", 7) == 0)
        {
            if (subcommand == NULL)
            {
                task_console_printf("ERROR: EEPROM command requires a subcommand\r\n");
                continue;
            }
            if (strncmp(subcommand, "r", 2) == 0)
            {
                uint8_t read_value = 0;
                if (system_sensors_eeprom_read(xQueue_Console_Rx_Responses, address_int, &read_value))
                {
                    task_console_printf("EEPROM READ: Address 0x%04X = 0x%02X\r\n", address_int, read_value);
                }
                else
                {
                    task_console_printf("ERROR: EEPROM Read Failed at Address 0x%04X\r\n", address_int);
                }
            }
            else if (strncmp(subcommand, "w", 2) == 0)
            {
                if (system_sensors_eeprom_write(xQueue_Console_Rx_Responses, address_int, value_int))
                {
                    task_console_printf("EEPROM WRITE: Addr=0x%04X, Value=0x%02X\r\n", address_int, value_int);
                }
                else
                {
                    task_console_printf("ERROR: EEPROM Write Failed at Address 0x%04X\r\n", address_int);
                }
            }
            else
            {
                task_console_printf("ERROR: Unknown EEPROM subcommand: %s\r\n", subcommand);
            }
        }
        else
        {
            task_console_printf("ERROR: Unknown command received: %s\r\n", command);
        }
    }
}


/**
 * @brief
 * This function initializes the resources for the console Rx task.
 * @return true if resources were initialized successfully
 * @return false if resource initialization failed
 */
//String one at a time, to generate a buffer 
//produce and consume buffer switch places eery time user hits the switch
//ISR writes to one buffer at a time, to erase race conditions.
bool task_console_resources_init_rx(void)
{
    BaseType_t rslt;

    /* ADD CODE */
    //Allocate an array of data from the heap for the console RX task.
    console_buffer1.data = (char *) pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);
    if(console_buffer1.data == NULL)
    {
        // Failed to allocate memory
        printf("Failed to allocate memory for console buffer 1\n");
        CY_ASSERT(0);
    }
    //console_buffer1.index = 0; //Initialize index to 0

    console_buffer2.data = (char *) pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);
    if(console_buffer2.data == NULL)
    {
        // Failed to allocate memory
        printf("Failed to allocate memory for console buffer 2\n");
        CY_ASSERT(0);
    }
    //console_buffer2.index = 0; //Initialize index to 0

    //Initialize the produce and consume buffer pointers
    produce_console_buffer = &console_buffer1;
    consume_console_buffer = &console_buffer2;

    //set console buffer indexes to 0
    produce_console_buffer->index = 0;
    consume_console_buffer->index = 0;

    //Create the queue used to receive device responses from gatekeeper tasks
    xQueue_Console_Rx_Responses = xQueueCreate(10, sizeof(device_response_msg_t));
    if(xQueue_Console_Rx_Responses == NULL)
    {
        printf("ERROR: Failed to create console Rx response queue\r\n");
        return false;
    }

    //Create the console Rx task
    rslt = xTaskCreate(task_console_rx, "Console Rx", 20*configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &TaskHandle_Console_Rx);

    return (rslt == pdPASS); // Resources initialized successfully
}
#endif