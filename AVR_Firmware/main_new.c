/*
# This file is part of the PiXtend(R) Project.
#
# For more information about PiXtend(R) and this program,
# see <http://www.pixtend.de> or <http://www.pixtend.com>
#
# Copyright (C) 2016 Tobias Gall, Christian Strobel
# Qube Solutions UG (haftungsbeschränkt), Luitgardweg 18
# 71083 Herrenberg, Germany 
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file main.c
 * @brief Main entry point for PiXtend firmware (ATmega32A)
 * 
 * This file contains the minimal main() function that initializes
 * hardware via HAL and delegates all business logic to the core module.
 * SPI interrupt handling is also implemented here.
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#include "hal/hal.h"
#include "core/pixtend_core.h"
#include "core/pixtend_types.h"

/*=============================================================================
 * Module Variables
 *============================================================================*/

/* SPI communication state */
static volatile uint8_t g_spi_mode = 0;              /* 0=idle, 1=manual, 2=automatic */
static volatile uint8_t g_spi_automatic_counter = 0;
static volatile uint8_t g_spi_data_mask = 0;
static volatile uint8_t g_spi_data_mask_set = 0;
static volatile uint8_t g_spi_transfer_complete = 0;
static volatile uint8_t g_spi_byte_cnt = 0;
static volatile uint8_t g_spi_transmit_temp = 0;

/* SPI buffers */
static volatile uint8_t g_spi_receive_buffer[PIXTEND_AUTOMATIC_MODE_BYTES];
static volatile uint8_t g_spi_transmit_buffer[PIXTEND_AUTOMATIC_MODE_BYTES];

/* Debug pin configuration */
#define GPIO_DEBUG 0

/*=============================================================================
 * SPI Interrupt Handler
 *============================================================================*/

ISR(SPI_STC_vect)
{
    uint8_t spi_receive_temp = hal_spi_get_data();
    
    /* Check for transfer completion */
    if (g_spi_transfer_complete == 1) {
        if (g_spi_mode == 2) {
            /* Automatic mode complete - signal to main loop */
            g_spi_transfer_complete = 0;
        }
        
        /* Reset state for next transfer */
        g_spi_mode = 0;
        g_spi_automatic_counter = 0;
        g_spi_data_mask_set = 0;
        g_spi_byte_cnt = 0;
        g_spi_transmit_temp = 0;
    }
    /* Automatic mode start */
    else if (spi_receive_temp == PIXTEND_SPI_CMD_AUTO_MODE && g_spi_mode == 0) {
        g_spi_mode = 2;
        g_spi_transmit_temp = PIXTEND_SPI_CMD_AUTO_MODE;  /* Acknowledge */
        g_spi_automatic_counter = 0;
    }
    /* Manual mode start */
    else if (spi_receive_temp == PIXTEND_SPI_CMD_MANUAL_MODE && g_spi_mode == 0) {
        g_spi_mode = 1;
        g_spi_transmit_temp = PIXTEND_SPI_CMD_MANUAL_MODE;  /* Acknowledge */
    }
    /* Automatic mode data transfer */
    else if (g_spi_mode == 2) {
        if (g_spi_automatic_counter == 0 && g_spi_data_mask_set == 0) {
            g_spi_data_mask = spi_receive_temp;
            g_spi_data_mask_set = 1;
            g_spi_transmit_temp = g_spi_transmit_buffer[0];
        } else if (g_spi_automatic_counter == PIXTEND_AUTOMATIC_MODE_BYTES - 1) {
            /* Last byte */
            g_spi_receive_buffer[g_spi_automatic_counter] = spi_receive_temp;
            g_spi_transmit_temp = 128;  /* End marker */
            g_spi_transfer_complete = 1;
        } else {
            g_spi_receive_buffer[g_spi_automatic_counter] = spi_receive_temp;
            g_spi_transmit_temp = g_spi_transmit_buffer[g_spi_automatic_counter + 1];
            g_spi_automatic_counter++;
        }
    }
    /* Manual mode command handling */
    else if (g_spi_mode == 1) {
        uint8_t response = 0;
        bool more_bytes = pixtend_manual_mode_execute(spi_receive_temp, spi_receive_temp, &response);
        
        if (!more_bytes) {
            g_spi_transmit_temp = response;
            g_spi_transfer_complete = 1;
        } else {
            g_spi_transmit_temp = response;
            g_spi_byte_cnt++;
        }
    }
    /* Unknown command - reset */
    else {
        g_spi_mode = 0;
        g_spi_transmit_temp = spi_receive_temp;
    }
    
    /* Send response byte */
    hal_spi_put_data(g_spi_transmit_temp);
}

/*=============================================================================
 * Main Function
 *============================================================================*/

int main(void)
{
    /* Initialize HAL */
    hal_system_init();
    hal_gpio_init();
    hal_dio_init();
    hal_adc_init();
    hal_pwm_init();
    hal_dht_init();
    hal_watchdog_init();
    hal_spi_init();
    
    /* Configure debug pin if enabled */
    if (GPIO_DEBUG) {
        hal_gpio_set_direction(0, true);
        pixtend_core_set_status_bit(PIXTEND_STATUS_DEBUG_MODE, true);
    }
    
    /* Initialize core business logic */
    pixtend_core_init();
    
    /* Enable SPI interrupt */
    SPCR = (1 << SPE) | (1 << SPIE);
    hal_interrupts_enable();
    
    /* Main loop */
    while (1) {
        /* Process automatic mode cycle if complete */
        if (g_spi_transfer_complete == 1 && g_spi_mode == 0) {
            if (GPIO_DEBUG) {
                hal_gpio_write(0, true);
            }
            
            hal_interrupts_disable();
            
            /* Process automatic mode */
            bool success = pixtend_automatic_mode_process(
                (const uint8_t*)g_spi_receive_buffer,
                (uint8_t*)g_spi_transmit_buffer
            );
            
            if (!success) {
                /* CRC error - trigger watchdog reset */
                hal_watchdog_enable(15);
                while (1) {
                    ;  /* Wait for watchdog reset */
                }
            }
            
            /* Reset watchdog and clear flag */
            hal_watchdog_reset();
            g_spi_transfer_complete = 0;
            
            hal_interrupts_enable();
            
            if (GPIO_DEBUG) {
                hal_gpio_write(0, false);
            }
        }
        
        /* Kick watchdog if enabled */
        if (pixtend_core_is_running()) {
            hal_watchdog_reset();
        }
    }
}
