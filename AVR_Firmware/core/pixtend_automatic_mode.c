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
 * @file pixtend_automatic_mode.c
 * @brief Automatic mode processing implementation
 */

#include "pixtend_automatic_mode.h"
#include "hal.h"
#include <string.h>

/*=============================================================================
 * Module Variables
 *============================================================================*/

/* DHT scheduling for automatic mode */
static uint8_t g_dht_scheduler_counter = 0;

/* Statistics */
static uint32_t g_successful_cycles = 0;
static uint32_t g_crc_errors = 0;

/*=============================================================================
 * Helper Functions
 *============================================================================*/

/**
 * @brief Apply GPIO configuration to hardware
 */
static void apply_gpio_config(const PixtendGpioConfig_t* config)
{
    for (uint8_t i = 0; i < PIXTEND_NUM_GPIO_PINS; i++) {
        if (config->dht_enable & (1 << (i + PIXTEND_GPIO_DHT_ENABLE_SHIFT))) {
            /* Pin is in DHT mode - handled by DHT driver */
            hal_dht_enable(i, true);
        } else if (config->direction & (1 << i)) {
            /* Pin is output */
            hal_gpio_set_direction(i, true);
        } else {
            /* Pin is input */
            hal_gpio_set_direction(i, false);
        }
    }
}

/**
 * @brief Apply PWM configuration to hardware
 */
static void apply_pwm_config(const PixtendPwmConfig_t* config, const uint16_t* pwm_values)
{
    hal_pwm_configure(
        config->prescaler,
        config->top_value,
        config->is_pwm_mode
    );
    
    /* If switching from servo to PWM mode, update duty cycles */
    if (config->is_pwm_mode) {
        hal_pwm_set_channel0(pwm_values[0]);
        hal_pwm_set_channel1(pwm_values[1]);
    }
}

/*=============================================================================
 * Public API Implementation
 *============================================================================*/

void pixtend_automatic_mode_init(void)
{
    g_dht_scheduler_counter = 0;
    pixtend_automatic_mode_reset_stats();
}

uint8_t pixtend_automatic_mode_update_dht_scheduler(void)
{
    g_dht_scheduler_counter++;
    
    /* Poll DHT sensors every 2 seconds (20 cycles @ 100ms) */
    if (g_dht_scheduler_counter >= 20) {
        g_dht_scheduler_counter = 0;
        
        /* Return next channel to poll (round-robin) */
        static uint8_t current_channel = 0;
        uint8_t channel_to_poll = current_channel;
        current_channel = (current_channel + 1) % PIXTEND_NUM_DHT_CHANNELS;
        
        return channel_to_poll;
    }
    
    return 255;  /* No polling needed */
}

bool pixtend_automatic_mode_process(const uint8_t* receive_buffer, uint8_t* transmit_buffer)
{
    /* Verify CRC */
    uint16_t crc_receive = 0xFFFF;
    for (uint8_t i = 0; i < (PIXTEND_AUTOMATIC_MODE_BYTES - 2); i++) {
        crc_receive = hal_crc16_update(crc_receive, receive_buffer[i]);
    }
    
    uint16_t crc_received = receive_buffer[29] | (receive_buffer[30] << 8);
    if (crc_received != crc_receive) {
        /* CRC error - data corrupted */
        g_crc_errors++;
        return false;
    }
    
    g_successful_cycles++;
    
    /* Get device state from core */
    PixtendDeviceState_t* state = pixtend_core_get_state();
    
    /* Only process if MCU is initialized (RUN state) */
    if (state->mcu_config.is_initialized) {
        /* Update control registers from received data */
        pixtend_core_set_gpio_config(receive_buffer[10]);
        pixtend_core_set_mcu_config(receive_buffer[11]);
        pixtend_core_set_adc_config(receive_buffer[12], receive_buffer[13]);
        pixtend_core_set_pwm_config(receive_buffer[7], receive_buffer[8], receive_buffer[9]);
        
        /* Update outputs */
        pixtend_core_set_digital_outputs(receive_buffer[0]);
        pixtend_core_set_relays(receive_buffer[1]);
        pixtend_core_set_gpio_outputs(receive_buffer[2]);
        pixtend_core_set_pwm_value(2, receive_buffer[3], receive_buffer[4]);  /* channel 2 = both */
        pixtend_core_set_pwm_value(2, receive_buffer[5], receive_buffer[6]);
        
        /* Read all inputs */
        pixtend_core_read_digital_inputs();
        pixtend_core_read_gpio_inputs();
        
        for (uint8_t i = 0; i < PIXTEND_NUM_ADC_CHANNELS; i++) {
            pixtend_core_read_adc(i);
        }
        
        /* Update DHT scheduler and read if needed */
        uint8_t dht_channel = pixtend_automatic_mode_update_dht_scheduler();
        if (dht_channel < PIXTEND_NUM_DHT_CHANNELS) {
            pixtend_core_read_dht(dht_channel);
        }
    } else {
        /* Check for INIT -> RUN transition request */
        if (receive_buffer[11] & PIXTEND_UC_CTRL_INIT_RUN) {
            pixtend_core_enter_run_state();
        }
    }
    
    /* Prepare response buffer */
    pixtend_automatic_mode_prepare_response(transmit_buffer);
    
    return true;
}

void pixtend_automatic_mode_prepare_response(uint8_t* transmit_buffer)
{
    PixtendDeviceState_t* state = pixtend_core_get_state();
    
    /* Digital inputs */
    transmit_buffer[0] = state->digital_inputs;
    
    /* ADC values (channels 0-3, bytes 1-8) */
    for (uint8_t i = 0; i < PIXTEND_NUM_ADC_CHANNELS; i++) {
        transmit_buffer[1 + 2*i] = state->adc_values[i].low;
        transmit_buffer[2 + 2*i] = state->adc_values[i].high;
    }
    
    /* Temperature values (bytes 10-17) */
    for (uint8_t i = 0; i < PIXTEND_NUM_DHT_CHANNELS; i++) {
        transmit_buffer[10 + 2*i] = state->temperature[i].low;
        transmit_buffer[11 + 2*i] = state->temperature[i].high;
    }
    
    /* Humidity values (bytes 18-25) */
    for (uint8_t i = 0; i < PIXTEND_NUM_DHT_CHANNELS; i++) {
        transmit_buffer[18 + 2*i] = state->humidity[i].low;
        transmit_buffer[19 + 2*i] = state->humidity[i].high;
    }
    
    /* GPIO inputs */
    transmit_buffer[9] = state->gpio_inputs;
    
    /* Version and status */
    transmit_buffer[26] = state->version_low;
    transmit_buffer[27] = state->version_high;
    transmit_buffer[28] = state->status_register;
    
    /* Calculate and append CRC */
    uint16_t crc_transmit = 0xFFFF;
    for (uint8_t i = 0; i < (PIXTEND_AUTOMATIC_MODE_BYTES - 2); i++) {
        crc_transmit = hal_crc16_update(crc_transmit, transmit_buffer[i]);
    }
    transmit_buffer[29] = crc_transmit & 0xFF;
    transmit_buffer[30] = crc_transmit >> 8;
}

void pixtend_automatic_mode_get_stats(uint32_t* successful_cycles, uint32_t* crc_errors)
{
    if (successful_cycles) {
        *successful_cycles = g_successful_cycles;
    }
    if (crc_errors) {
        *crc_errors = g_crc_errors;
    }
}

void pixtend_automatic_mode_reset_stats(void)
{
    g_successful_cycles = 0;
    g_crc_errors = 0;
}
