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
 * @file pixtend_core.h
 * @brief Hardware-independent business logic for PiXtend firmware
 * 
 * This module contains device state management and low-level operations:
 * - Device state initialization and access
 * - Configuration update functions
 * - Output control functions
 * - Input acquisition functions
 * - State machine management
 * 
 * For SPI protocol handling, see:
 * - pixtend_automatic_mode.h - Automatic mode processing
 * - pixtend_manual_mode.h - Manual mode command handling
 */

#ifndef PIXTEND_CORE_H
#define PIXTEND_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "pixtend_types.h"

/*=============================================================================
 * Core Initialization
 *============================================================================*/

/**
 * @brief Initialize PiXtend core module
 * 
 * Resets device state to defaults and prepares for operation.
 * Must be called before any other core functions.
 */
void pixtend_core_init(void);

/**
 * @brief Get pointer to current device state
 * @return Pointer to PixtendDeviceState_t structure
 */
PixtendDeviceState_t* pixtend_core_get_state(void);

/*=============================================================================
 * Configuration Update Functions
 *============================================================================*/

/**
 * @brief Update GPIO configuration
 * @param config New GPIO configuration register value
 */
void pixtend_core_set_gpio_config(uint8_t config);

/**
 * @brief Update PWM configuration
 * @param reg0 PWM control register 0
 * @param reg1 PWM control register 1 (low byte of TOP)
 * @param reg2 PWM control register 2 (high byte of TOP)
 */
void pixtend_core_set_pwm_config(uint8_t reg0, uint8_t reg1, uint8_t reg2);

/**
 * @brief Update ADC configuration
 * @param reg0 ADC control register 0 (sample rates)
 * @param reg1 ADC control register 1 (prescaler)
 */
void pixtend_core_set_adc_config(uint8_t reg0, uint8_t reg1);

/**
 * @brief Update MCU control configuration
 * @param config MCU control register value
 */
void pixtend_core_set_mcu_config(uint8_t config);

/*=============================================================================
 * Output Control Functions
 *============================================================================*/

/**
 * @brief Set digital output states
 * @param value Bitmask of output states (bits 0-3)
 */
void pixtend_core_set_digital_outputs(uint8_t value);

/**
 * @brief Set relay output states
 * @param value Bitmask of relay states (bits 0-3)
 */
void pixtend_core_set_relays(uint8_t value);

/**
 * @brief Set GPIO output states
 * @param value Bitmask of GPIO output states (bits 0-3)
 */
void pixtend_core_set_gpio_outputs(uint8_t value);

/**
 * @brief Set PWM/Servo value
 * @param channel Channel number (0 or 1)
 * @param value_low Low byte of duty cycle
 * @param value_high High byte of duty cycle
 */
void pixtend_core_set_pwm_value(uint8_t channel, uint8_t value_low, uint8_t value_high);

/*=============================================================================
 * Input Acquisition Functions
 *============================================================================*/

/**
 * @brief Read all digital inputs and update state
 */
void pixtend_core_read_digital_inputs(void);

/**
 * @brief Read all GPIO inputs and update state
 */
void pixtend_core_read_gpio_inputs(void);

/**
 * @brief Read ADC channel and update state
 * @param channel ADC channel number (0-3)
 */
void pixtend_core_read_adc(uint8_t channel);

/**
 * @brief Read DHT sensor and update state
 * @param channel DHT channel number (0-3)
 */
void pixtend_core_read_dht(uint8_t channel);

/**
 * @brief Read digital outputs (feedback) and update state
 */
void pixtend_core_read_digital_outputs(void);

/**
 * @brief Read relay outputs (feedback) and update state
 */
void pixtend_core_read_relays(void);

/*=============================================================================
 * State Machine Management
 *============================================================================*/

/**
 * @brief Transition from INIT to RUN state
 * @return true if transition successful
 */
bool pixtend_core_enter_run_state(void);

/**
 * @brief Check if device is in RUN state
 * @return true if initialized and running
 */
bool pixtend_core_is_running(void);

/**
 * @brief Set status bit in MCU status register
 * @param bit Bit position (0-7)
 * @param value Bit value (0 or 1)
 */
void pixtend_core_set_status_bit(uint8_t bit, bool value);

/**
 * @brief Get MCU status register value
 * @return Current status register value
 */
uint8_t pixtend_core_get_status_register(void);

#endif /* PIXTEND_CORE_H */
