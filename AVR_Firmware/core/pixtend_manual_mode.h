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
 * @file pixtend_manual_mode.h
 * @brief Manual mode command handling interface
 * 
 * This module handles the manual mode SPI communication protocol.
 * In manual mode, individual commands are sent to read/write specific
 * registers and values.
 */

#ifndef PIXTEND_MANUAL_MODE_H
#define PIXTEND_MANUAL_MODE_H

#include <stdint.h>
#include <stdbool.h>
#include "pixtend_types.h"

/*=============================================================================
 * Manual Mode API
 *============================================================================*/

/**
 * @brief Initialize manual mode module
 * 
 * Resets internal state machine and prepares for manual mode operation.
 * Must be called during system initialization.
 */
void pixtend_manual_mode_init(void);

/**
 * @brief Execute manual mode command
 * 
 * Processes a single manual mode SPI command and returns response byte.
 * This function implements the complete manual mode command set.
 * 
 * @param command Command byte received from SPI
 * @param data Data byte received from SPI (for write commands)
 * @param response Pointer to store response byte
 * @return true if more bytes expected for this command, false if complete
 */
bool pixtend_manual_mode_execute(uint8_t command, uint8_t data, uint8_t* response);

/**
 * @brief Reset manual mode state machine
 * 
 * Called when manual mode transfer completes or times out.
 * Clears all temporary state variables.
 */
void pixtend_manual_mode_reset(void);

/**
 * @brief Get manual mode statistics
 * @param commands_processed Pointer to store command count
 * @param errors Pointer to store error count
 */
void pixtend_manual_mode_get_stats(uint32_t* commands_processed, uint32_t* errors);

/**
 * @brief Reset manual mode statistics
 */
void pixtend_manual_mode_reset_stats(void);

#endif /* PIXTEND_MANUAL_MODE_H */
