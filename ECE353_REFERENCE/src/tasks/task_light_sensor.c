/**
 * @file task_light_sensor.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ECE353_FREERTOS)
#include "drivers.h"
#include "task_light_sensor.h"
#include "task_console.h"
#include "rtos_events.h"

extern EventGroupHandle_t ECE353_RTOS_Events;

#define TASK_LIGHT_SENSOR_STACK_SIZE (5*configMINIMAL_STACK_SIZE)
#define TASK_LIGHT_SENSOR_PRIORITY   (tskIDLE_PRIORITY + 1)

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/
/* I2C Object Handle */
static cyhal_i2c_t *I2C_Obj;

/* I2C Semaphore */
static SemaphoreHandle_t *I2C_Semaphore = NULL;

/* Queue used to send commands used to light sensor */
QueueHandle_t Queue_Light_Sensor_Requests;


/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/
/**
 * @brief
 * Set ALS MODE to Active and initiate a software reset
 */
static void ltr_light_sensor_start(void)
{

    /* ADD CODE */
    // We initialize most likely
    //I2C_Obj = i2c_init(LTR_SUBORDINATE_ADDR, LTR_SUBORDINATE_ADDR);
    //i2c_write_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, LTR_REG_CONTR_ALS_MODE | LTR_REG_CONTR_SW_RESET);
    // separate ALS MODE and SW reset with a delay . Reset first then ALS MODE
    i2c_write_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, LTR_REG_CONTR_SW_RESET);
    vTaskDelay(pdMS_TO_TICKS(100));
    i2c_write_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, LTR_REG_CONTR_ALS_MODE);



}

static uint8_t ltr_light_get_contr(void)
{
    uint8_t value = 0;
    
    /* ADD CODE */
    //There are 2 bytes upper and lower byte
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, &value);



    return value;
}

static uint8_t ltr_light_sensor_status(void)
{
    uint8_t value = 0;

    /* ADD CODE */
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_STATUS, &value);

    return value;
}

/**
 * @brief
 * Returns the part ID of the LTR_329ALS-01
 * @return uint8_t
 */
static uint8_t ltr_light_sensor_part_id(void)
{
    uint8_t value = 0;

    /* ADD CODE */
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_PART_ID, &value);

    return value;
}

static uint8_t ltr_light_sensor_manufac_id(void)
{
    uint8_t value = 0;

    /* ADD CODE */
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_MANUFAC_ID, &value);

    return value;
}

static uint16_t ltr_light_sensor_get_ch0(void)
{
    uint8_t msbyte;
    uint8_t lsbyte;
    uint16_t value = 0;

    /* ADD CODE */
    //i2c_read_u16(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH0_0, &value);
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH0_0, &lsbyte);
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH0_1, &msbyte);
    value = (msbyte << 8) | lsbyte;

    return value;
}

static uint16_t ltr_light_sensor_get_ch1(void)
{
    uint8_t msbyte;
    uint8_t lsbyte;
    uint16_t value = 0;

    /* ADD CODE */
    //i2c_read_u16(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH1_0, &value);
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH1_0, &lsbyte);
    i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH1_1, &msbyte);
    value = (msbyte << 8) | lsbyte;

    return value;
}

static void ltr_light_sensor_get_readings(uint16_t *ch1, uint16_t *ch0)
{
    uint8_t status = 0;

    status = ltr_light_sensor_status();
    while((status & LTR_REG_STATUS_NEW_DATA) != LTR_REG_STATUS_NEW_DATA)
    {
        // Wait
        status = ltr_light_sensor_status();
    }

    *ch1 = ltr_light_sensor_get_ch1();
    *ch0 = ltr_light_sensor_get_ch0();
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/
/*
* @brief 
 * This is a helper function that is called by tasks other than the light sensor task
 * when they want to read the ambient light from the sensor.
*/
bool system_sensors_get_light(QueueHandle_t return_queue, uint16_t *ambient_light)
{
    bool status = false;

    /* ADD CODE */
    device_request_msg_t request;
    device_response_msg_t response;
    request.device = DEVICE_LIGHT;
    request.operation = DEVICE_OP_READ;
    request.address = 0; // Not used
    request.value = 0;   // Not used
    request.response_queue = return_queue;
    if(xQueueSend(Queue_Light_Sensor_Requests, &request, portMAX_DELAY) == pdPASS)
    {
        // Wait for response
        if(xQueueReceive(return_queue, &response, pdMS_TO_TICKS(100)) == pdPASS)
        {
            if(response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
            {
                *ambient_light = response.payload.light_sensor;
                status = true;
            }
        }

    }

    return status;
}



/**
 * @brief
 * Task used to monitor the reception of command packets sent the light sensor
 * @param param
 * Unused
 */
void task_light_sensor(void *param)
{
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;

	task_console_printf("Starting Light Sensor Task\r\n");

	uint8_t manufac_id = ltr_light_sensor_manufac_id();
    if(manufac_id != 0x05)
    {
        task_console_printf("Light Sensor Manufacturer ID Invalid: 0x%02X\r\n", manufac_id);
        vTaskSuspend(NULL);
    }
    else {
        task_console_printf("Light Sensor Manufacturer ID Valid: 0x%02X\r\n", manufac_id);
    }

	// grab the semaphore to access the bus
	xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
	ltr_light_sensor_start();
	// release the semaphore after accessing the bus
	xSemaphoreGive(*I2C_Semaphore);

	while (1)
	{
        // ADD CODE
        xQueueReceive(Queue_Light_Sensor_Requests, &request_packet, portMAX_DELAY);
        if(request_packet.operation == DEVICE_OP_READ)
        {
            uint16_t ch0 = 0;
            uint16_t ch1 = 0;

            // grab the semaphore to access the bus
            xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
            ltr_light_sensor_get_readings(&ch1, &ch0);
            // release the semaphore after accessing the bus
            xSemaphoreGive(*I2C_Semaphore);

            response_packet.device = DEVICE_LIGHT;
            response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
            response_packet.payload.light_sensor = ch0; // Return channel 0 as ambient light

            // Send response back to requesting task
            xQueueSend(request_packet.response_queue, &response_packet, pdMS_TO_TICKS(100));
        }
    }
	
}

/**
 * @brief
 * Initializes software resources related to the operation of
 * the Temp Sensor.  This function expects that the I2C bus had already
 * been initialized prior to the start of FreeRTOS.
 */
bool task_light_sensor_resources_init(cyhal_i2c_t *i2c_obj, SemaphoreHandle_t *i2c_semaphore)
{
    /* Save the I2C Object */
    I2C_Obj = i2c_obj;

    /* Save the I2C Semaphore */
    I2C_Semaphore = i2c_semaphore;
    if (I2C_Semaphore == NULL)
    {
        return false;
    }

	/* Create the Queue used to receive requests from other tasks */
	Queue_Light_Sensor_Requests = xQueueCreate(1, sizeof(device_request_msg_t));
	if (Queue_Light_Sensor_Requests == NULL)
	{
		return false;
	}
	
	/* Create the task that will control the status LED */
	if(xTaskCreate(
		task_light_sensor,
		"Light Sensor",
		TASK_LIGHT_SENSOR_STACK_SIZE,
		NULL,
		TASK_LIGHT_SENSOR_PRIORITY,
		NULL) != pdPASS)
	{
		return false;
	}
	else 
	{
		return true;
	}	
}	
#endif