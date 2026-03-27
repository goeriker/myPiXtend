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
 * @file pixtend_manual_mode.c
 * @brief Manual mode command handling implementation
 */

#include "pixtend_manual_mode.h"
#include "hal.h"
#include <string.h>

/*=============================================================================
 * Module Variables
 *============================================================================*/

/* Manual mode state machine */
static uint8_t g_manual_command = 0;
static uint8_t g_manual_byte_count = 0;
static uint16_t g_manual_pwm_temp = 0;
static uint8_t g_manual_pwm_ctrl_temp[3] = {0};
static uint8_t g_manual_adc_ctrl_temp[2] = {0};

/* Statistics */
static uint32_t g_commands_processed = 0;
static uint32_t g_errors = 0;

/*=============================================================================
 * Public API Implementation
 *============================================================================*/

void pixtend_manual_mode_init(void)
{
    pixtend_manual_mode_reset();
    pixtend_manual_mode_reset_stats();
}

bool pixtend_manual_mode_execute(uint8_t command, uint8_t data, uint8_t* response)
{
    *response = data;  /* Default: echo back data */
    g_commands_processed++;
    
    /* Write commands (single byte) */
    if (command == PIXTEND_CMD_WRITE_DO) {
        pixtend_core_set_digital_outputs(data);
        return false;  /* Complete */
    }
    
    if (command == PIXTEND_CMD_WRITE_RELAYS) {
        pixtend_core_set_relays(data);
        return false;
    }
    
    if (command == PIXTEND_CMD_WRITE_GPIO) {
        pixtend_core_set_gpio_outputs(data);
        return false;
    }
    
    /* Read commands (single byte or multi-byte) */
    if (command == PIXTEND_CMD_READ_DI) {
        pixtend_core_read_digital_inputs();
        *response = pixtend_core_get_state()->digital_inputs;
        return false;
    }
    
    if (command == PIXTEND_CMD_READ_GPIO) {
        pixtend_core_read_gpio_inputs();
        *response = pixtend_core_get_state()->gpio_inputs;
        return false;
    }
    
    if (command == PIXTEND_CMD_READ_DO) {
        pixtend_core_read_digital_outputs();
        *response = pixtend_core_get_state()->digital_outputs;
        return false;
    }
    
    if (command == PIXTEND_CMD_READ_RELAYS) {
        pixtend_core_read_relays();
        *response = pixtend_core_get_state()->relay_outputs;
        return false;
    }
    
    /* Multi-byte read commands (ADC, Temperature, Humidity) */
    if (command >= PIXTEND_CMD_READ_ADC0 && command <= PIXTEND_CMD_READ_ADC3) {
        uint8_t channel = command - PIXTEND_CMD_READ_ADC0;
        if (g_manual_byte_count == 0) {
            *response = pixtend_core_get_state()->adc_values[channel].low;
            g_manual_byte_count = 1;
            return true;  /* Expect more bytes */
        } else {
            *response = pixtend_core_get_state()->adc_values[channel].high;
            g_manual_byte_count = 0;
            /* Schedule ADC read for background */
            pixtend_core_read_adc(channel);
            return false;
        }
    }
    
    if (command >= PIXTEND_CMD_READ_TEMP0 && command <= PIXTEND_CMD_READ_TEMP3) {
        uint8_t channel = command - PIXTEND_CMD_READ_TEMP0;
        if (g_manual_byte_count == 0) {
            *response = pixtend_core_get_state()->temperature[channel].low;
            g_manual_byte_count = 1;
            return true;
        } else {
            *response = pixtend_core_get_state()->temperature[channel].high;
            g_manual_byte_count = 0;
            pixtend_core_read_dht(channel);
            return false;
        }
    }
    
    if (command >= PIXTEND_CMD_READ_HUMID0 && command <= PIXTEND_CMD_READ_HUMID3) {
        uint8_t channel = command - PIXTEND_CMD_READ_HUMID0;
        if (g_manual_byte_count == 0) {
            *response = pixtend_core_get_state()->humidity[channel].low;
            g_manual_byte_count = 1;
            return true;
        } else {
            *response = pixtend_core_get_state()->humidity[channel].high;
            g_manual_byte_count = 0;
            /* Temperature and humidity are read together */
            return false;
        }
    }
    
    /* Register write commands */
    if (command == PIXTEND_CMD_WRITE_SERVO0) {
        pixtend_core_set_pwm_value(0, data, 0);
        return false;
    }
    
    if (command == PIXTEND_CMD_WRITE_SERVO1) {
        pixtend_core_set_pwm_value(1, data, 0);
        return false;
    }
    
    if (command == PIXTEND_CMD_WRITE_PWM0 || command == PIXTEND_CMD_WRITE_PWM1) {
        if (g_manual_byte_count == 0) {
            g_manual_pwm_temp = data;
            g_manual_byte_count = 1;
            return true;
        } else {
            uint8_t channel = (command == PIXTEND_CMD_WRITE_PWM0) ? 0 : 1;
            pixtend_core_set_pwm_value(channel, g_manual_pwm_temp & 0xFF, data);
            g_manual_byte_count = 0;
            return false;
        }
    }
    
    if (command == PIXTEND_CMD_WRITE_PWM_CTRL) {
        if (g_manual_byte_count < 3) {
            g_manual_pwm_ctrl_temp[g_manual_byte_count++] = data;
            if (g_manual_byte_count == 3) {
                pixtend_core_set_pwm_config(g_manual_pwm_ctrl_temp[0], 
                                           g_manual_pwm_ctrl_temp[1],
                                           g_manual_pwm_ctrl_temp[2]);
                g_manual_byte_count = 0;
            }
            return true;
        }
        return false;
    }
    
    if (command == PIXTEND_CMD_WRITE_GPIO_CTRL) {
        pixtend_core_set_gpio_config(data);
        return false;
    }
    
    if (command == PIXTEND_CMD_WRITE_UC_CTRL) {
        pixtend_core_set_mcu_config(data);
        return false;
    }
    
    if (command == PIXTEND_CMD_WRITE_ADC_CTRL) {
        if (g_manual_byte_count == 0) {
            g_manual_adc_ctrl_temp[0] = data;
            g_manual_byte_count = 1;
            return true;
        } else {
            pixtend_core_set_adc_config(g_manual_adc_ctrl_temp[0], data);
            g_manual_byte_count = 0;
            return false;
        }
    }
    
    if (command == PIXTEND_CMD_WRITE_RASPI_STATUS) {
        /* Reserved for future use */
        return false;
    }
    
    /* Register read commands */
    if (command == PIXTEND_CMD_READ_VERSION) {
        if (g_manual_byte_count == 0) {
            *response = pixtend_core_get_state()->version_low;
            g_manual_byte_count = 1;
            return true;
        } else {
            *response = pixtend_core_get_state()->version_high;
            g_manual_byte_count = 0;
            return false;
        }
    }
    
    if (command == PIXTEND_CMD_READ_UC_STATUS) {
        *response = pixtend_core_get_state()->status_register;
        return false;
    }
    
    /* Unknown command */
    g_errors++;
    return false;
}

void pixtend_manual_mode_reset(void)
{
    g_manual_command = 0;
    g_manual_byte_count = 0;
    g_manual_pwm_temp = 0;
    g_manual_pwm_ctrl_temp[0] = 0;
    g_manual_pwm_ctrl_temp[1] = 0;
    g_manual_pwm_ctrl_temp[2] = 0;
    g_manual_adc_ctrl_temp[0] = 0;
    g_manual_adc_ctrl_temp[1] = 0;
}

void pixtend_manual_mode_get_stats(uint32_t* commands_processed, uint32_t* errors)
{
    if (commands_processed) {
        *commands_processed = g_commands_processed;
    }
    if (errors) {
        *errors = g_errors;
    }
}

void pixtend_manual_mode_reset_stats(void)
{
    g_commands_processed = 0;
    g_errors = 0;
}
