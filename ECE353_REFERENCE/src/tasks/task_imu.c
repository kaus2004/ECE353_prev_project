/**
 * @file task_imu.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-16
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "task_imu.h"

 #if defined(ECE353_FREERTOS)
#include "imu.h"
#include "task_console.h"

#define TASK_IMU_STACK_SIZE (configMINIMAL_STACK_SIZE * 5 )
#define TASK_IMU_PRIORITY   (tskIDLE_PRIORITY + 1)

static SemaphoreHandle_t *SPI_Semaphore = NULL;
static cyhal_spi_t *imu_spi_obj = NULL;
static cyhal_gpio_t imu_cs_pin = NC;

extern QueueHandle_t Queue_IMU_Requests=NULL;

/**
 * @brief
 * Helper function for other tasks to request IMU data
 * 
 * @param return_queue Queue to receive the response
 * @param accel Array to store accelerometer data [x, y, z]
 * @return true if successful, false otherwise
 */
bool system_sensors_get_imu(QueueHandle_t return_queue, int16_t *accel)
{
    bool status = false;
    device_request_msg_t request;
    device_response_msg_t response;
    
    request.device = DEVICE_IMU;
    request.operation = DEVICE_OP_READ;
    request.address = 0;
    request.value = 0;
    request.response_queue = return_queue;
    
    if(xQueueSend(Queue_IMU_Requests, &request, portMAX_DELAY) == pdPASS)
    {
        // Wait for response
        if(xQueueReceive(return_queue, &response, pdMS_TO_TICKS(500)) == pdPASS)
        {
            if(response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
            {
                accel[0] = response.payload.imu[0];
                accel[1] = response.payload.imu[1];
                accel[2] = response.payload.imu[2];
                status = true;
            }
        }else{
            return pdFALSE;
        }
    }
    
    return status;
}

 /**
  * @brief 
  * This function will create the IMU task for reading data from the IMU sensor.
  * It assumes that you have already created a semaphore for SPI access and initialized
  * the SPI peripheral.  This function does NOT initialize the SPI peripheral OR CS Pin 
  * because the SPI peripheral is shared between multiple tasks (e.g. IMU, EEPROM, etc.). 
  * @param spi_semaphore 
  * @return true 
  * @return false 
  */
 bool task_imu_resources_init(void *spi_semaphore, cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
 {
  // more syncronized way:
  SPI_Semaphore = (SemaphoreHandle_t *)spi_semaphore;
  imu_spi_obj = spi_obj;
  imu_cs_pin = cs_pin;
  
  /* Create the FreeRTOS task for the IMU */
  if (xTaskCreate(
      task_imu, 
      "IMU", 
      TASK_IMU_STACK_SIZE, 
      spi_semaphore, 
      TASK_IMU_PRIORITY, 
      NULL) != pdPASS)  
    {
      return false;
    }

   // Create the IMU Requests Queue
    Queue_IMU_Requests = xQueueCreate(10, sizeof(device_request_msg_t));
    if (Queue_IMU_Requests == NULL)
    {
        return false;
    }

   return true;
 }

 void task_imu(void *arg)
 {
    (void) arg;
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;

    // Array to store the raw accelerometer data
    int16_t accel_data[3]; // to see neg and pos values
    int16_t gyro_data[3]; // to see neg and pos values

    xSemaphoreTake(*SPI_Semaphore, portMAX_DELAY);

    if(!imu_init(imu_spi_obj, imu_cs_pin))
    {
        task_console_printf("IMU Initialization Failed!\r\n");
        vTaskDelete(NULL);
    }else
    {
        task_console_printf("IMU Initialization Succeeded!\r\n");
    }
    xSemaphoreGive(*SPI_Semaphore);

  
    while(1)
    {
        //Receive the queue
        xQueueReceive(Queue_IMU_Requests, &request_packet, portMAX_DELAY);
        // Add code here
        vTaskDelay(pdMS_TO_TICKS(250));

        if(request_packet.operation == DEVICE_OP_READ)
        {
            //grab the semaphore
            xSemaphoreTake(*SPI_Semaphore, portMAX_DELAY);

            //read the accelerometer data
            imu_read_registers(imu_spi_obj, imu_cs_pin, IMU_REG_OUTX_L_XL , (uint8_t *)accel_data, 6);

            //read the gyroscope data
            imu_read_registers(imu_spi_obj, imu_cs_pin, IMU_REG_OUTX_L_G , (uint8_t *)gyro_data, 6);

            //next 6 bytes are gyro data, we are not using it here

            //Print Accelerometer Data
            //Give SPI semaphore
            //release the semaphore
            xSemaphoreGive(*SPI_Semaphore);

            //task_console_printf("Accel X: %d, Y: %d, Z: %d\r\n", accel_data[0], accel_data[1], accel_data[2]);
            //task_console_printf("Gyro  X: %d, Y: %d, Z: %d\r\n", gyro_data[0], gyro_data[1], gyro_data[2]);
   
            // After reading accel_data and gyro_data
            //task_sensor_fusion(accel_data, gyro_data);


            //prepare the response packet
            response_packet.device = DEVICE_IMU;
            response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS; // successful read
            //no we already have data printed we done here'
            
            response_packet.payload.imu[0] = accel_data[0];
            response_packet.payload.imu[1] = accel_data[1];
            response_packet.payload.imu[2] = accel_data[2];

            //send the response back to the requesting task
        }
        xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
    }
}
#endif /* ECE353_FREERTOS */