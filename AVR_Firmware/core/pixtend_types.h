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
 * @file pixtend_types.h
 * @brief Common type definitions and constants for PiXtend firmware
 * 
 * This header provides hardware-independent type definitions, constants,
 * and data structures used throughout the PiXtend firmware.
 */

#ifndef PIXTEND_TYPES_H
#define PIXTEND_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/*=============================================================================
 * Version Information
 *============================================================================*/
#define PIXTEND_VERSION_LOW     2       /*!< Firmware version low byte */
#define PIXTEND_VERSION_HIGH    13      /*!< Firmware version high byte */

/*=============================================================================
 * Hardware Configuration Constants
 *============================================================================*/
#define PIXTEND_NUM_ADC_CHANNELS        4       /*!< Number of ADC channels */
#define PIXTEND_NUM_DHT_CHANNELS        4       /*!< Number of DHT sensor channels */
#define PIXTEND_NUM_DIGITAL_INPUTS      4       /*!< Number of digital inputs */
#define PIXTEND_NUM_DIGITAL_OUTPUTS     4       /*!< Number of digital outputs */
#define PIXTEND_NUM_RELAY_OUTPUTS       4       /*!< Number of relay outputs */
#define PIXTEND_NUM_GPIO_PINS           4       /*!< Number of GPIO pins */
#define PIXTEND_NUM_PWM_CHANNELS        2       /*!< Number of PWM/Servo channels */

/*=============================================================================
 * SPI Communication Constants
 *============================================================================*/
#define PIXTEND_AUTOMATIC_MODE_BYTES    31      /*!< Number of bytes in automatic mode transfer */
#define PIXTEND_SPI_CMD_AUTO_MODE       0x80    /*!< Automatic mode command */
#define PIXTEND_SPI_CMD_MANUAL_MODE     0xAA    /*!< Manual mode command */

/*=============================================================================
 * Manual Mode Command Definitions
 *============================================================================*/
/* Write commands */
#define PIXTEND_CMD_WRITE_DO            0x01    /*!< Write digital outputs */
#define PIXTEND_CMD_WRITE_RELAYS        0x07    /*!< Write relay outputs */
#define PIXTEND_CMD_WRITE_GPIO          0x08    /*!< Write GPIO outputs */
#define PIXTEND_CMD_WRITE_SERVO0        0x80    /*!< Write servo channel 0 */
#define PIXTEND_CMD_WRITE_SERVO1        0x81    /*!< Write servo channel 1 */
#define PIXTEND_CMD_WRITE_PWM0          0x82    /*!< Write PWM channel 0 */
#define PIXTEND_CMD_WRITE_PWM1          0x83    /*!< Write PWM channel 1 */
#define PIXTEND_CMD_WRITE_PWM_CTRL      0x84    /*!< Write PWM control registers */
#define PIXTEND_CMD_WRITE_GPIO_CTRL     0x85    /*!< Write GPIO control register */
#define PIXTEND_CMD_WRITE_UC_CTRL       0x86    /*!< Write MCU control register */
#define PIXTEND_CMD_WRITE_ADC_CTRL      0x87    /*!< Write ADC control registers */
#define PIXTEND_CMD_WRITE_RASPI_STATUS  0x88    /*!< Write Raspberry status register */

/* Read commands */
#define PIXTEND_CMD_READ_DI             0x02    /*!< Read digital inputs */
#define PIXTEND_CMD_READ_ADC0           0x03    /*!< Read ADC channel 0 */
#define PIXTEND_CMD_READ_ADC1           0x04    /*!< Read ADC channel 1 */
#define PIXTEND_CMD_READ_ADC2           0x05    /*!< Read ADC channel 2 */
#define PIXTEND_CMD_READ_ADC3           0x06    /*!< Read ADC channel 3 */
#define PIXTEND_CMD_READ_GPIO           0x09    /*!< Read GPIO inputs */
#define PIXTEND_CMD_READ_TEMP0          0x0A    /*!< Read temperature channel 0 */
#define PIXTEND_CMD_READ_TEMP1          0x0B    /*!< Read temperature channel 1 */
#define PIXTEND_CMD_READ_TEMP2          0x0C    /*!< Read temperature channel 2 */
#define PIXTEND_CMD_READ_TEMP3          0x0D    /*!< Read temperature channel 3 */
#define PIXTEND_CMD_READ_HUMID0         0x0E    /*!< Read humidity channel 0 */
#define PIXTEND_CMD_READ_HUMID1         0x0F    /*!< Read humidity channel 1 */
#define PIXTEND_CMD_READ_HUMID2         0x10    /*!< Read humidity channel 2 */
#define PIXTEND_CMD_READ_HUMID3         0x11    /*!< Read humidity channel 3 */
#define PIXTEND_CMD_READ_DO             0x12    /*!< Read digital outputs */
#define PIXTEND_CMD_READ_RELAYS         0x13    /*!< Read relay outputs */
#define PIXTEND_CMD_READ_VERSION        0x89    /*!< Read MCU version */
#define PIXTEND_CMD_READ_UC_STATUS      0x8A    /*!< Read MCU status */

/*=============================================================================
 * Control Register Bit Definitions
 *============================================================================*/
/* GPIO Control Register */
#define PIXTEND_GPIO_CTRL_MODE_MASK     0xF0    /*!< GPIO mode mask (upper 4 bits) */
#define PIXTEND_GPIO_CTRL_DIR_MASK      0x0F    /*!< GPIO direction mask (lower 4 bits) */
#define PIXTEND_GPIO_DHT_ENABLE_SHIFT   4       /*!< DHT enable bit shift */

/* PWM Control Register 0 */
#define PIXTEND_PWM_CTRL_ENABLE         0x01    /*!< PWM mode enable bit */
#define PIXTEND_PWM_CTRL_OVERDRIVE0     0x02    /*!< Servo 0 overdrive enable */
#define PIXTEND_PWM_CTRL_OVERDRIVE1     0x04    /*!< Servo 1 overdrive enable */
#define PIXTEND_PWM_CTRL_PRESCALER_MASK 0xE0    /*!< Prescaler selection mask */
#define PIXTEND_PWM_CTRL_PRESCALER_SHIFT 5      /*!< Prescaler selection shift */

/* MCU Control Register */
#define PIXTEND_UC_CTRL_WATCHDOG        0x01    /*!< Watchdog enable bit */
#define PIXTEND_UC_CTRL_INIT_RUN        0x10    /*!< INIT to RUN state transition */

/* ADC Sample Rate Options */
#define PIXTEND_ADC_SAMPLES_10          0x00    /*!< 10 samples per conversion */
#define PIXTEND_ADC_SAMPLES_1           0x01    /*!< 1 sample per conversion */
#define PIXTEND_ADC_SAMPLES_5           0x02    /*!< 5 samples per conversion */
#define PIXTEND_ADC_SAMPLES_50          0x03    /*!< 50 samples per conversion */

/*=============================================================================
 * Status Register Bit Definitions
 *============================================================================*/
#define PIXTEND_STATUS_INIT_STATE       0       /*!< Bit 0: INIT state indicator */
#define PIXTEND_STATUS_DEBUG_MODE       1       /*!< Bit 1: Debug mode indicator */

/*=============================================================================
 * Error Codes
 *============================================================================*/
#define PIXTEND_ERR_OK                  0       /*!< No error */
#define PIXTEND_ERR_DHT_BUSY            1       /*!< DHT bus is busy */
#define PIXTEND_ERR_DHT_TIMEOUT_START   2       /*!< DHT start signal timeout */
#define PIXTEND_ERR_DHT_TIMEOUT_RESP_L  3       /*!< DHT response low timeout */
#define PIXTEND_ERR_DHT_TIMEOUT_RESP_H  4       /*!< DHT response high timeout */
#define PIXTEND_ERR_DHT_TIMEOUT_BIT_L   5       /*!< DHT bit low timeout */
#define PIXTEND_ERR_DHT_TIMEOUT_BIT_H   6       /*!< DHT bit high timeout */
#define PIXTEND_ERR_DHT_CHECKSUM        7       /*!< DHT checksum error */
#define PIXTEND_ERR_DHT_DISABLED        8       /*!< DHT sensor disabled */

/*=============================================================================
 * Data Structures
 *============================================================================*/

/**
 * @brief ADC value storage (16-bit value in two 8-bit arrays)
 */
typedef struct {
    uint8_t low;    /*!< Low byte of ADC value */
    uint8_t high;   /*!< High byte of ADC value */
} PixtendAdcValue_t;

/**
 * @brief Temperature/Humidity sensor value storage
 */
typedef struct {
    uint8_t low;    /*!< Low byte of sensor value */
    uint8_t high;   /*!< High byte of sensor value */
} PixtendSensorValue_t;

/**
 * @brief PWM configuration parameters
 */
typedef struct {
    uint8_t prescaler;      /*!< Timer prescaler setting */
    uint16_t top_value;     /*!< TOP value for PWM period */
    bool is_pwm_mode;       /*!< true = PWM mode, false = Servo mode */
    bool overdrive_ch0;     /*!< Servo channel 0 overdrive enabled */
    bool overdrive_ch1;     /*!< Servo channel 1 overdrive enabled */
} PixtendPwmConfig_t;

/**
 * @brief ADC configuration parameters
 */
typedef struct {
    uint8_t sample_rate[PIXTEND_NUM_ADC_CHANNELS];  /*!< Sample rate per channel */
    uint8_t prescaler;                              /*!< ADC prescaler setting */
} PixtendAdcConfig_t;

/**
 * @brief GPIO configuration parameters
 */
typedef struct {
    uint8_t direction;      /*!< Pin direction (bitmask: 1=output, 0=input) */
    uint8_t dht_enable;     /*!< DHT sensor enable (bitmask) */
} PixtendGpioConfig_t;

/**
 * @brief MCU control parameters
 */
typedef struct {
    bool watchdog_enabled;  /*!< Watchdog timer enabled */
    bool is_initialized;    /*!< MCU initialized (RUN state) */
} PixtendMcuConfig_t;

/**
 * @brief Complete device state structure
 * 
 * This structure contains all input/output values and configuration
 * for the PiXtend device. It serves as the central data model.
 */
typedef struct {
    /* Input Values */
    uint8_t digital_inputs;                         /*!< Digital input states */
    uint8_t gpio_inputs;                            /*!< GPIO input states */
    PixtendAdcValue_t adc_values[PIXTEND_NUM_ADC_CHANNELS];     /*!< ADC values */
    PixtendSensorValue_t temperature[PIXTEND_NUM_DHT_CHANNELS]; /*!< Temperature values */
    PixtendSensorValue_t humidity[PIXTEND_NUM_DHT_CHANNELS];    /*!< Humidity values */
    
    /* Output Values */
    uint8_t digital_outputs;                        /*!< Digital output states */
    uint8_t relay_outputs;                          /*!< Relay output states */
    uint8_t gpio_outputs;                           /*!< GPIO output states */
    uint16_t pwm_values[PIXTEND_NUM_PWM_CHANNELS];  /*!< PWM/Servo values */
    
    /* Configuration */
    PixtendGpioConfig_t gpio_config;                /*!< GPIO configuration */
    PixtendPwmConfig_t pwm_config;                  /*!< PWM configuration */
    PixtendAdcConfig_t adc_config;                  /*!< ADC configuration */
    PixtendMcuConfig_t mcu_config;                  /*!< MCU configuration */
    
    /* Status */
    uint8_t status_register;                        /*!< MCU status register */
    uint8_t version_low;                            /*!< Firmware version low */
    uint8_t version_high;                           /*!< Firmware version high */
} PixtendDeviceState_t;

#endif /* PIXTEND_TYPES_H */
