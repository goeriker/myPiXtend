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
 * @file hal.h
 * @brief Hardware Abstraction Layer interface definitions
 * 
 * This header defines the hardware abstraction layer interface that must be
 * implemented for each target MCU platform. The business logic depends only
 * on these interfaces, not on specific hardware implementations.
 */

#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "pixtend_types.h"

/*=============================================================================
 * GPIO HAL Interface
 *============================================================================*/

/**
 * @brief Initialize GPIO pins for PiXtend
 * 
 * Configures all GPIO pins used by PiXtend (GPIO 0-3, digital I/O, etc.)
 */
void hal_gpio_init(void);

/**
 * @brief Set GPIO pin direction
 * @param pin Pin number (0-3 for GPIO pins)
 * @param is_output true = output, false = input
 */
void hal_gpio_set_direction(uint8_t pin, bool is_output);

/**
 * @brief Write value to GPIO output pin
 * @param pin Pin number (0-3)
 * @param value Value to write (0 or 1)
 */
void hal_gpio_write(uint8_t pin, bool value);

/**
 * @brief Read value from GPIO input pin
 * @param pin Pin number (0-3)
 * @return Pin value (0 or 1)
 */
bool hal_gpio_read(uint8_t pin);

/**
 * @brief Read all GPIO pins at once
 * @return Bitmask of GPIO pin states (bits 0-3)
 */
uint8_t hal_gpio_read_all(void);

/**
 * @brief Write to multiple GPIO outputs at once
 * @param value Bitmask of values to write (bits 0-3)
 */
void hal_gpio_write_all(uint8_t value);

/*=============================================================================
 * Digital I/O HAL Interface
 *============================================================================*/

/**
 * @brief Initialize digital input/output pins
 */
void hal_dio_init(void);

/**
 * @brief Read all digital inputs
 * @return Bitmask of digital input states (bits 0-3)
 */
uint8_t hal_digital_inputs_read(void);

/**
 * @brief Write to all digital outputs
 * @param value Bitmask of values to write (bits 0-3)
 */
void hal_digital_outputs_write(uint8_t value);

/**
 * @brief Read back digital output states
 * @return Bitmask of current digital output states
 */
uint8_t hal_digital_outputs_read(void);

/**
 * @brief Write to all relay outputs
 * @param value Bitmask of relay states (bits 0-3)
 */
void hal_relays_write(uint8_t value);

/**
 * @brief Read back relay output states
 * @return Bitmask of current relay states
 */
uint8_t hal_relays_read(void);

/*=============================================================================
 * ADC HAL Interface
 *============================================================================*/

/**
 * @brief Initialize ADC peripheral
 */
void hal_adc_init(void);

/**
 * @brief Set ADC prescaler
 * @param prescaler Prescaler value (platform-specific)
 */
void hal_adc_set_prescaler(uint8_t prescaler);

/**
 * @brief Read ADC channel (blocking)
 * @param channel Channel number (0-3)
 * @return 10-bit ADC value (0-1023)
 */
uint16_t hal_adc_read(uint8_t channel);

/*=============================================================================
 * PWM/Servo HAL Interface
 *============================================================================*/

/**
 * @brief Initialize PWM/Servo timer peripheral
 */
void hal_pwm_init(void);

/**
 * @brief Configure PWM/Servo parameters
 * @param prescaler Timer prescaler value
 * @param top_value TOP value for timer period
 * @param is_pwm_mode true = PWM mode, false = Servo mode
 */
void hal_pwm_configure(uint8_t prescaler, uint16_t top_value, bool is_pwm_mode);

/**
 * @brief Set PWM/Servo duty cycle for channel 0
 * @param duty_cycle 16-bit duty cycle value
 */
void hal_pwm_set_channel0(uint16_t duty_cycle);

/**
 * @brief Set PWM/Servo duty cycle for channel 1
 * @param duty_cycle 16-bit duty cycle value
 */
void hal_pwm_set_channel1(uint16_t duty_cycle);

/*=============================================================================
 * DHT Sensor HAL Interface
 *============================================================================*/

/**
 * @brief Initialize DHT sensor interface
 */
void hal_dht_init(void);

/**
 * @brief Enable DHT mode on GPIO pin
 * @param channel DHT channel number (0-3)
 * @param enabled true = enable DHT mode, false = disable
 */
void hal_dht_enable(uint8_t channel, bool enabled);

/**
 * @brief Read DHT sensor (blocking with timeouts)
 * @param channel DHT channel number (0-3)
 * @param humidity Pointer to store humidity value (x10 for DHT11, x10 for DHT22)
 * @param temperature Pointer to store temperature value (x10 for DHT11, x10 for DHT22)
 * @return Error code (PIXTEND_ERR_OK on success)
 */
uint8_t hal_dht_read(uint8_t channel, uint16_t* humidity, uint16_t* temperature);

/*=============================================================================
 * Watchdog HAL Interface
 *============================================================================*/

/**
 * @brief Initialize watchdog timer
 */
void hal_watchdog_init(void);

/**
 * @brief Enable watchdog timer
 * @param timeout_ms Timeout in milliseconds
 */
void hal_watchdog_enable(uint16_t timeout_ms);

/**
 * @brief Disable watchdog timer
 */
void hal_watchdog_disable(void);

/**
 * @brief Reset (kick) watchdog timer
 */
void hal_watchdog_reset(void);

/*=============================================================================
 * SPI HAL Interface
 *============================================================================*/

/**
 * @brief Initialize SPI peripheral
 */
void hal_spi_init(void);

/**
 * @brief Enable SPI interrupt
 */
void hal_spi_enable_interrupt(void);

/**
 * @brief Disable SPI interrupt
 */
void hal_spi_disable_interrupt(void);

/**
 * @brief Get received byte from SPI data register
 * @return Received byte
 */
uint8_t hal_spi_get_data(void);

/**
 * @brief Write byte to SPI data register for transmission
 * @param data Byte to transmit
 */
void hal_spi_put_data(uint8_t data);

/*=============================================================================
 * System HAL Interface
 *============================================================================*/

/**
 * @brief Initialize system clock and basic peripherals
 */
void hal_system_init(void);

/**
 * @brief Enable global interrupts
 */
void hal_interrupts_enable(void);

/**
 * @brief Disable global interrupts
 */
void hal_interrupts_disable(void);

/**
 * @brief Delay in microseconds
 * @param us Microseconds to delay
 */
void hal_delay_us(uint16_t us);

/**
 * @brief Delay in milliseconds
 * @param ms Milliseconds to delay
 */
void hal_delay_ms(uint16_t ms);

/**
 * @brief Calculate CRC16 checksum
 * @param crc Previous CRC value
 * @param data Data byte to process
 * @return Updated CRC value
 */
uint16_t hal_crc16_update(uint16_t crc, uint8_t data);

#endif /* HAL_H */
