/**
 * @file imu.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "imu.h"
#include "cy_result.h"
#include "cyhal_hw_types.h"
#include <stdio.h>

/**
 * @brief 
 * Read a register from the IMU
 * @param reg 
 * @param value 
 */
void imu_write_reg(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg, 
    uint8_t value
)
{
    // write two bytes of data: register address and value 
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2];
    tx_buffer[0] = reg & 0x7F; // Clear MSB for write operation
    tx_buffer[1] = value;
    //pull chip select low
    cyhal_gpio_write(cs_pin, 0); // Assert CS

    // Send register address and value
    cyhal_spi_transfer(spi_obj, tx_buffer, 2, rx_buffer, 2, 0xFF);

    //pull chip select high
    cyhal_gpio_write(cs_pin, 1); // De-assert CS

}

/**
 * @brief 
 * Read a register from the IMU
 * @param reg 
 * @return uint8_t 
 */
uint8_t imu_read_reg(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg
)
{
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2];
    tx_buffer[0] = reg | 0x80; // Set MSB for read operation
    tx_buffer[1] = 0xFF; // Dummy byte

    //pull chip select low
    cyhal_gpio_write(cs_pin, 0); // Assert CS

    // Send register address and read value
    cyhal_spi_transfer(spi_obj, tx_buffer, 2, rx_buffer, 2, 0xFF);

    //pull chip select high
    cyhal_gpio_write(cs_pin, 1); // De-assert CS

    return rx_buffer[1];
}

/**
 * @brief Read multiple registers from the IMU
 * 
 * @param reg 
 * @param buffer 
 * @param length 
 */
void imu_read_registers(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg, 
    uint8_t *buffer, 
    uint8_t length
)
{
    uint8_t tx_buffer[1+length];
    uint8_t rx_buffer[1+length];
    tx_buffer[0] = reg | 0x80; // Set MSB for read operation and auto-increment
    //printf("TX0 = 0x%02X (%d)\r\n", tx_buffer[0], tx_buffer[0]);

    cyhal_gpio_write(cs_pin, 0);
    cyhal_spi_transfer(spi_obj, tx_buffer, 1 + length, rx_buffer, 1+length, 0xFF);

    // Copy received data to buffer, skipping the first byte (register address)
    cyhal_gpio_write(cs_pin, 1); // Assert CS

    memcpy(buffer, &rx_buffer[1], length);

 
}

/**
 * @brief 
 * This function verifys the WHO_AM_I register and configures the IMU
 * to 104 Hz, ±2g, ±250 dps.
 * @param spi_obj 
 * @param cs_pin 
 * @return true 
 * @return false 
 */
bool imu_init(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
    // Wait for 15mS after power-up
    cyhal_system_delay_ms(15);

    // Verify the WHO_AM_I register
    uint8_t who_am_i = imu_read_reg(spi_obj, cs_pin, IMU_REG_WHO_AM_I);
    if (who_am_i != 0x6A)
    {
        return false;
    }

    // Reset the IMU
    imu_write_reg(spi_obj, cs_pin, IMU_REG_CTRL3_C, 0x01); // CTRL3_C register

    // Wait for the reset to complete
    do
    {
        cyhal_system_delay_ms(1);
    } while (imu_read_reg(spi_obj, cs_pin, IMU_REG_CTRL3_C) & 0x01);

    cyhal_system_delay_ms(15);

    //Tried adding BDU but nothing happened
    imu_write_reg(spi_obj, cs_pin, IMU_REG_CTRL3_C, 0x44);

    // Configure the IMU:  104 Hz, ±2g, ±250 dps (accelerometer)
    imu_write_reg(spi_obj, cs_pin, IMU_REG_CTRL1_XL, ODR_104HZ | FS_XL_2G);

    // Configure the IMU:  104 Hz, ±250 dps (gyroscope) independently configured
    imu_write_reg(spi_obj, cs_pin, IMU_REG_CTRL2_G, ODR_104HZ | FS_G_250DPS);

    return true;
}