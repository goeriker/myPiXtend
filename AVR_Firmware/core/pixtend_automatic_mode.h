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
 * @file pixtend_automatic_mode.h
 * @brief Automatic mode processing interface
 * 
 * This module handles the automatic mode SPI communication protocol.
 * In automatic mode, the Raspberry Pi sends 31 bytes of data and receives
 * current sensor values and status information.
 */

#ifndef PIXTEND_AUTOMATIC_MODE_H
#define PIXTEND_AUTOMATIC_MODE_H

#include <stdint.h>
#include <stdbool.h>
#include "pixtend_types.h"

/*=============================================================================
 * Automatic Mode API
 *============================================================================*/

/**
 * @brief Initialize automatic mode module
 * 
 * Resets internal state and prepares for automatic mode operation.
 * Must be called during system initialization.
 */
void pixtend_automatic_mode_init(void);

/**
 * @brief Process automatic mode cycle
 * 
 * This function is called when an automatic mode SPI transfer completes.
 * It validates CRC, updates outputs, reads inputs, and prepares response.
 * 
 * @param receive_buffer Pointer to received SPI data (31 bytes)
 * @param transmit_buffer Pointer to transmit buffer (31 bytes)
 * @return true if cycle processed successfully, false on CRC error
 */
bool pixtend_automatic_mode_process(const uint8_t* receive_buffer, uint8_t* transmit_buffer);

/**
 * @brief Prepare automatic mode transmit buffer
 * 
 * Fills transmit buffer with current input values and status.
 * Can be called independently to refresh output data.
 * 
 * @param transmit_buffer Pointer to transmit buffer (31 bytes)
 */
void pixtend_automatic_mode_prepare_response(uint8_t* transmit_buffer);

/**
 * @brief Update DHT sensor scheduling counter
 * 
 * In automatic mode, DHT sensors are polled every 2 seconds.
 * This function should be called each automatic cycle (100ms).
 * 
 * @return Channel to poll (0-3), or 255 if no polling needed
 */
uint8_t pixtend_automatic_mode_update_dht_scheduler(void);

/**
 * @brief Get automatic mode statistics
 * @param successful_cycles Pointer to store successful cycle count
 * @param crc_errors Pointer to store CRC error count
 */
void pixtend_automatic_mode_get_stats(uint32_t* successful_cycles, uint32_t* crc_errors);

/**
 * @brief Reset automatic mode statistics
 */
void pixtend_automatic_mode_reset_stats(void);

#endif /* PIXTEND_AUTOMATIC_MODE_H */
