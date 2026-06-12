/**
 * @file eeprom.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2023-10-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "eeprom.h"
#include "cyhal_hw_types.h"
#include <sys/types.h>


/** Determine if the EEPROM is busy writing the last
 *  transaction to non-volatile storage
 *
 * @param
 *
 */
void eeprom_wait_for_write(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	// ADD CODE
	// Use the chip select since chip select goes low to start a transaction.
	// Mode 0. The clock is active high or active low: active high, Phase 0 1st clock edge
	//read chip select and then start

	//cyhal_gpio_read(cs_pin); //wait for CS to go low
	// give RDSR command to read status register

	//I'll make buffer 
	uint8_t buffer_tx[2];
	uint8_t buffer_rx[2];
	uint8_t cmd = EEPROM_CMD_RDSR;
	uint8_t status = 0;

	buffer_tx[0] =cmd;
	buffer_tx[1]=0;

	buffer_rx[0] = 0;
	buffer_rx[1] = 0;

	do{
		cyhal_gpio_write(cs_pin, 0);
		cyhal_spi_transfer(spi_obj, buffer_tx, 2, buffer_rx, 2, 0xFF);
		cyhal_gpio_write(cs_pin, 1);

    }while(buffer_rx[1] & 0x01); //waiting for previous write to complete
	// read status register
	// check WIP bit

}

/** Enables Writes to the EEPROM
 *
 * @param
 *
 */
void eeprom_write_enable(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	// ADD CODE
	uint8_t buffer_tx[2];
	uint8_t buffer_rx[2];
	buffer_tx[0] = EEPROM_CMD_WREN;
	buffer_tx[1] = 0;

	buffer_rx[0] = 0;
	buffer_rx[1] = 0;

	cyhal_gpio_write(cs_pin, 0);
	cyhal_spi_transfer(spi_obj, buffer_tx, 2, buffer_rx, 2, 0xFF);
	cyhal_gpio_write(cs_pin, 1);

}

/** Disable Writes to the EEPROM
 *
 * @param
 *
 */
void eeprom_write_disable(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	// ADD CODE
	uint8_t buffer_tx[2];
	uint8_t buffer_rx[2];
	buffer_tx[0] = EEPROM_CMD_WRDI;
	buffer_tx[1] = 0;

	buffer_rx[0] = 0;
	buffer_rx[1] = 0;

	cyhal_gpio_write(cs_pin, 0);
	cyhal_spi_transfer(spi_obj, buffer_tx, 2, buffer_rx, 2, 0xFF);
	cyhal_gpio_write(cs_pin, 1);

}

/** Writes a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 * @param data    -- value to write into memory
 *
 */
void eeprom_write_byte(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin, uint16_t address, uint8_t data)
{
    // Step 1: WREN
    eeprom_write_enable(spi_obj, cs_pin);

    // Step 2: WRITE command
    uint8_t buffer_tx[4];
    uint8_t buffer_rx[4] = {0};

    buffer_tx[0] = EEPROM_CMD_WRITE;
    buffer_tx[1] = (address >> 8) & 0xFF;
    buffer_tx[2] = address & 0xFF;
    buffer_tx[3] = data;

    cyhal_gpio_write(cs_pin, 0);
    cyhal_spi_transfer(spi_obj, buffer_tx, 4, buffer_rx, 4, 0xFF);
    cyhal_gpio_write(cs_pin, 1);

    // Step 3: Wait until write completes
    eeprom_wait_for_write(spi_obj, cs_pin);
}


/** Reads a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 *
 */
uint8_t eeprom_read_byte(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin, uint16_t address)
{
	// ADD CODE
	uint8_t buffer_tx[4];
	uint8_t buffer_rx[4];
	buffer_tx[0] = EEPROM_CMD_READ;
	buffer_tx[1] = (address >> 8) & 0xFF; //High byte
	buffer_tx[2] = address & 0xFF; //Low byte
	buffer_tx[3] = 0; //dummy byte
	buffer_rx[0] = 0;
	buffer_rx[1] = 0;
	buffer_rx[2] = 0;
	buffer_rx[3] = 0;
	cyhal_gpio_write(cs_pin, 0);
	cyhal_spi_transfer(spi_obj, buffer_tx, 4, buffer_rx, 4, 0xFF);
	cyhal_gpio_write(cs_pin, 1);
	return buffer_rx[3];
}