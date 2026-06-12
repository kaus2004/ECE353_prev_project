/**
 * @file task_temp_sensor.c
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
#include "cy_result.h"
#include "drivers.h"
#include "task_temp_sensor.h"
#include "task_console.h"

#define TASK_TEMP_SENSOR_STACK_SIZE (configMINIMAL_STACK_SIZE)
#define TASK_TEMP_SENSOR_PRIORITY   (tskIDLE_PRIORITY + 1)

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
//gives us exclusive access to the i2c bus

/* Queue used to send commands used to temp sensor */
QueueHandle_t Queue_Temp_Sensor_Requests;
//if anyone makes a request this is the queue that our gatekeeper task will monitor

/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/

/** Read the value of the input port
 *
 * @param reg The reg address to read
 *
 */
static float LM75_get_temp(void)
{
	float temp = 0.0f;

	/* ADD CODE */	
	cy_rslt_t rslt;
	int16_t raw_temp = 0;
	rslt = i2c_read_u16(I2C_Obj, LM75_SUBORDINATE_ADDR, LM75_TEMP_REG,(uint16_t *) &raw_temp);
	//as temperature requires signed numbers
	if(rslt != CY_RSLT_SUCCESS)
	{
		// Convert raw temp to celsius
	    return 0;
	}
	//convert to celsius

	temp = (float)(raw_temp >> 7) * 0.5f;

	return temp;
}

static uint8_t LM75_get_product_id(void)
{
	uint8_t prod_id = 0;

	/* ADD CODE */	
	cy_rslt_t rslt;
	rslt = i2c_read_u8(I2C_Obj, LM75_SUBORDINATE_ADDR, LM75_PRODUCT_ID_REG, &prod_id);
	if(rslt != CY_RSLT_SUCCESS)
	{
	    return 0;
	}

	return prod_id;
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

/**
 * @brief 
 * This is a helper function that is called by tasks other than the temp sensor task
 * when they want to read the temperature from the sensor.
 */
bool system_sensors_get_temp(QueueHandle_t return_queue, float *temperature)
{
	device_request_msg_t packet;
	device_response_msg_t response;

	if(return_queue == NULL || temperature == NULL)
	{
		return false;
	}

	/* ADD CODE */
	packet.device = DEVICE_TEMPERATURE;
	packet.operation = DEVICE_OP_READ;
	packet.response_queue = return_queue;
	
	
	/* Send request to the temp sensor task */
    if (xQueueSend(Queue_Temp_Sensor_Requests, &packet, pdMS_TO_TICKS(200)) != pdTRUE) {
        // send failed (queue full or not ready)
        return false;
    }

	/* Wait for the response */
    if (xQueueReceive(return_queue, &response, pdMS_TO_TICKS(1000)) != pdTRUE) {
        // timed out waiting for response
        return false;
    }

	*temperature = response.payload.temperature;


	if(response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
	{
		
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @brief
 * Task used to monitor the reception of command packets sent the temp sensor
 * @param param
 * Unused
 */
void task_temp_sensor(void *param)
{
	device_request_msg_t request_packet;
	device_response_msg_t response_packet;
	
	task_console_printf("Starting Temp Sensor Task\r\n");

	// Take the I2C semaphore to configure the temp sensor
	xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
	
	// Verify that the temp sensor is connected by reading the product ID
	uint8_t prod_id = LM75_get_product_id();
	if(prod_id == LM75_PRODUCT_ID)
	{
		printf("Temp Sensor Detected! Product ID: 0x%02X\r\n", prod_id);
	}
	else
	{
		printf("Temp Sensor NOT Detected! 0x%02X\r\n", prod_id);
		vTaskDelay(pdMS_TO_TICKS(1000));
		CY_ASSERT(0);
	}

	// give the semaphore back
	xSemaphoreGive(*I2C_Semaphore);	

	while (1)
	{
		/* Wait for a message */
		xQueueReceive(Queue_Temp_Sensor_Requests, &request_packet, portMAX_DELAY);

		/* ADD CODE */

		//grab i2c semaphore
		xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);

		//read temp
		float temperature = LM75_get_temp();

		//release i2c semaphore
		xSemaphoreGive(*I2C_Semaphore);

		//return the temperature
		response_packet.device = DEVICE_TEMPERATURE;
		response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
		response_packet.payload.temperature = temperature;
		xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
	}
}

/**
 * @brief
 * Initializes software resources related to the operation of
 * the Temp Sensor.  This function expects that the I2C bus had already
 * been initialized prior to the start of FreeRTOS.
 */
bool task_temp_sensor_resources_init(cyhal_i2c_t *i2c_obj, SemaphoreHandle_t *i2c_semaphore)
{
	/* Save the I2C object and semaphore */
	I2C_Obj = i2c_obj;
	I2C_Semaphore = i2c_semaphore;

	if (I2C_Semaphore == NULL || I2C_Obj == NULL)
	{
		return false;
	}	

	/* Create the Queue used to receive requests  */
	Queue_Temp_Sensor_Requests = xQueueCreate(1, sizeof(device_request_msg_t));
	if (Queue_Temp_Sensor_Requests == NULL)
	{
		return false;
	}
	
	/* Create the task that will control the status LED */
	if(xTaskCreate(
		task_temp_sensor,
		"Temp Sensor",
		TASK_TEMP_SENSOR_STACK_SIZE,
		i2c_semaphore,
		TASK_TEMP_SENSOR_PRIORITY,
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