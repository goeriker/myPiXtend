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
 * @file pixtend_core.c
 * @brief Hardware-independent business logic implementation
 */

#include "pixtend_core.h"
#include "hal.h"
#include <string.h>

/*=============================================================================
 * Module Variables
 *============================================================================*/

static PixtendDeviceState_t g_device_state;
static bool g_is_initialized = false;

/* DHT scheduling for automatic mode */
static uint8_t g_dht_scheduler_counter = 0;

/* Manual mode state machine */
static uint8_t g_manual_command = 0;
static uint8_t g_manual_byte_count = 0;
static uint16_t g_manual_pwm_temp = 0;
static uint8_t g_manual_pwm_ctrl_temp[3] = {0};
static uint8_t g_manual_adc_ctrl_temp[2] = {0};

/*=============================================================================
 * Helper Functions
 *============================================================================*/

/**
 * @brief Calculate number of ADC samples based on configuration
 */
static uint8_t get_adc_samples(uint8_t config_value)
{
    switch (config_value & 0x03) {
        case PIXTEND_ADC_SAMPLES_10:
            return 10;
        case PIXTEND_ADC_SAMPLES_1:
            return 1;
        case PIXTEND_ADC_SAMPLES_5:
            return 5;
        case PIXTEND_ADC_SAMPLES_50:
            return 50;
        default:
            return 10;
    }
}

/**
 * @brief Apply GPIO configuration to hardware
 */
static void apply_gpio_config(void)
{
    for (uint8_t i = 0; i < PIXTEND_NUM_GPIO_PINS; i++) {
        if (g_device_state.gpio_config.dht_enable & (1 << (i + PIXTEND_GPIO_DHT_ENABLE_SHIFT))) {
            /* Pin is in DHT mode - handled by DHT driver */
            hal_dht_enable(i, true);
        } else if (g_device_state.gpio_config.direction & (1 << i)) {
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
static void apply_pwm_config(void)
{
    hal_pwm_configure(
        g_device_state.pwm_config.prescaler,
        g_device_state.pwm_config.top_value,
        g_device_state.pwm_config.is_pwm_mode
    );
    
    /* If switching from servo to PWM mode, update duty cycles */
    if (g_device_state.pwm_config.is_pwm_mode) {
        hal_pwm_set_channel0(g_device_state.pwm_values[0]);
        hal_pwm_set_channel1(g_device_state.pwm_values[1]);
    }
}

/*=============================================================================
 * Core Initialization
 *============================================================================*/

void pixtend_core_init(void)
{
    /* Reset device state to defaults */
    memset(&g_device_state, 0, sizeof(PixtendDeviceState_t));
    
    /* Set version information */
    g_device_state.version_low = PIXTEND_VERSION_LOW;
    g_device_state.version_high = PIXTEND_VERSION_HIGH;
    
    /* Default PWM config: Servo mode, 20ms period */
    g_device_state.pwm_config.is_pwm_mode = false;
    g_device_state.pwm_config.top_value = 5000;
    g_device_state.pwm_config.prescaler = 3;  /* Prescaler 64 */
    
    /* Default ADC config: 10 samples, prescaler 128 */
    for (uint8_t i = 0; i < PIXTEND_NUM_ADC_CHANNELS; i++) {
        g_device_state.adc_config.sample_rate[i] = PIXTEND_ADC_SAMPLES_10;
    }
    g_device_state.adc_config.prescaler = 7;  /* Prescaler 128 */
    
    /* Reset DHT scheduler */
    g_dht_scheduler_counter = 0;
    
    /* Reset manual mode state */
    pixtend_manual_mode_reset();
    
    g_is_initialized = true;
}

PixtendDeviceState_t* pixtend_core_get_state(void)
{
    return &g_device_state;
}

/*=============================================================================
 * Automatic Mode Processing
 *============================================================================*/

bool pixtend_automatic_mode_process(const uint8_t* receive_buffer, uint8_t* transmit_buffer)
{
    if (!g_is_initialized) {
        return false;
    }
    
    /* Verify CRC */
    uint16_t crc_receive = 0xFFFF;
    for (uint8_t i = 0; i < (PIXTEND_AUTOMATIC_MODE_BYTES - 2); i++) {
        crc_receive = hal_crc16_update(crc_receive, receive_buffer[i]);
    }
    
    uint16_t crc_received = receive_buffer[29] | (receive_buffer[30] << 8);
    if (crc_received != crc_receive) {
        /* CRC error - data corrupted */
        return false;
    }
    
    /* Only process if MCU is initialized (RUN state) */
    if (g_device_state.mcu_config.is_initialized) {
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
        uint8_t dht_channel = pixtend_core_update_dht_scheduler();
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
    /* Digital inputs */
    transmit_buffer[0] = g_device_state.digital_inputs;
    
    /* ADC values (channels 0-3, bytes 1-8) */
    for (uint8_t i = 0; i < PIXTEND_NUM_ADC_CHANNELS; i++) {
        transmit_buffer[1 + 2*i] = g_device_state.adc_values[i].low;
        transmit_buffer[2 + 2*i] = g_device_state.adc_values[i].high;
    }
    
    /* Temperature values (bytes 10-17) */
    for (uint8_t i = 0; i < PIXTEND_NUM_DHT_CHANNELS; i++) {
        transmit_buffer[10 + 2*i] = g_device_state.temperature[i].low;
        transmit_buffer[11 + 2*i] = g_device_state.temperature[i].high;
    }
    
    /* Humidity values (bytes 18-25) */
    for (uint8_t i = 0; i < PIXTEND_NUM_DHT_CHANNELS; i++) {
        transmit_buffer[18 + 2*i] = g_device_state.humidity[i].low;
        transmit_buffer[19 + 2*i] = g_device_state.humidity[i].high;
    }
    
    /* GPIO inputs */
    transmit_buffer[9] = g_device_state.gpio_inputs;
    
    /* Version and status */
    transmit_buffer[26] = g_device_state.version_low;
    transmit_buffer[27] = g_device_state.version_high;
    transmit_buffer[28] = g_device_state.status_register;
    
    /* Calculate and append CRC */
    uint16_t crc_transmit = 0xFFFF;
    for (uint8_t i = 0; i < (PIXTEND_AUTOMATIC_MODE_BYTES - 2); i++) {
        crc_transmit = hal_crc16_update(crc_transmit, transmit_buffer[i]);
    }
    transmit_buffer[29] = crc_transmit & 0xFF;
    transmit_buffer[30] = crc_transmit >> 8;
}

/*=============================================================================
 * Manual Mode Command Handling
 *============================================================================*/

bool pixtend_manual_mode_execute(uint8_t command, uint8_t data, uint8_t* response)
{
    *response = data;  /* Default: echo back data */
    
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
        *response = g_device_state.digital_inputs;
        return false;
    }
    
    if (command == PIXTEND_CMD_READ_GPIO) {
        pixtend_core_read_gpio_inputs();
        *response = g_device_state.gpio_inputs;
        return false;
    }
    
    if (command == PIXTEND_CMD_READ_DO) {
        pixtend_core_read_digital_outputs();
        *response = g_device_state.digital_outputs;
        return false;
    }
    
    if (command == PIXTEND_CMD_READ_RELAYS) {
        pixtend_core_read_relays();
        *response = g_device_state.relay_outputs;
        return false;
    }
    
    /* Multi-byte read commands (ADC, Temperature, Humidity) */
    if (command >= PIXTEND_CMD_READ_ADC0 && command <= PIXTEND_CMD_READ_ADC3) {
        uint8_t channel = command - PIXTEND_CMD_READ_ADC0;
        if (g_manual_byte_count == 0) {
            *response = g_device_state.adc_values[channel].low;
            g_manual_byte_count = 1;
            return true;  /* Expect more bytes */
        } else {
            *response = g_device_state.adc_values[channel].high;
            g_manual_byte_count = 0;
            /* Schedule ADC read for background */
            pixtend_core_read_adc(channel);
            return false;
        }
    }
    
    if (command >= PIXTEND_CMD_READ_TEMP0 && command <= PIXTEND_CMD_READ_TEMP3) {
        uint8_t channel = command - PIXTEND_CMD_READ_TEMP0;
        if (g_manual_byte_count == 0) {
            *response = g_device_state.temperature[channel].low;
            g_manual_byte_count = 1;
            return true;
        } else {
            *response = g_device_state.temperature[channel].high;
            g_manual_byte_count = 0;
            pixtend_core_read_dht(channel);
            return false;
        }
    }
    
    if (command >= PIXTEND_CMD_READ_HUMID0 && command <= PIXTEND_CMD_READ_HUMID3) {
        uint8_t channel = command - PIXTEND_CMD_READ_HUMID0;
        if (g_manual_byte_count == 0) {
            *response = g_device_state.humidity[channel].low;
            g_manual_byte_count = 1;
            return true;
        } else {
            *response = g_device_state.humidity[channel].high;
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
            *response = g_device_state.version_low;
            g_manual_byte_count = 1;
            return true;
        } else {
            *response = g_device_state.version_high;
            g_manual_byte_count = 0;
            return false;
        }
    }
    
    if (command == PIXTEND_CMD_READ_UC_STATUS) {
        *response = g_device_state.status_register;
        return false;
    }
    
    /* Unknown command */
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

/*=============================================================================
 * Configuration Update Functions
 *============================================================================*/

void pixtend_core_set_gpio_config(uint8_t config)
{
    g_device_state.gpio_config.direction = config & PIXTEND_GPIO_CTRL_DIR_MASK;
    g_device_state.gpio_config.dht_enable = config & PIXTEND_GPIO_CTRL_MODE_MASK;
    apply_gpio_config();
}

void pixtend_core_set_pwm_config(uint8_t reg0, uint8_t reg1, uint8_t reg2)
{
    uint8_t old_prescaler = g_device_state.pwm_config.prescaler;
    bool old_mode = g_device_state.pwm_config.is_pwm_mode;
    
    g_device_state.pwm_config.prescaler = (reg0 >> PIXTEND_PWM_CTRL_PRESCALER_SHIFT) & 0x07;
    g_device_state.pwm_config.is_pwm_mode = (reg0 & PIXTEND_PWM_CTRL_ENABLE) != 0;
    g_device_state.pwm_config.overdrive_ch0 = (reg0 & PIXTEND_PWM_CTRL_OVERDRIVE0) != 0;
    g_device_state.pwm_config.overdrive_ch1 = (reg0 & PIXTEND_PWM_CTRL_OVERDRIVE1) != 0;
    g_device_state.pwm_config.top_value = (reg2 << 8) | reg1;
    
    /* Apply configuration if changed */
    if ((old_prescaler != g_device_state.pwm_config.prescaler && g_device_state.pwm_config.is_pwm_mode) ||
        (old_mode && !g_device_state.pwm_config.is_pwm_mode)) {
        apply_pwm_config();
    }
}

void pixtend_core_set_adc_config(uint8_t reg0, uint8_t reg1)
{
    for (uint8_t i = 0; i < PIXTEND_NUM_ADC_CHANNELS; i++) {
        g_device_state.adc_config.sample_rate[i] = (reg0 >> (2 * i)) & 0x03;
    }
    g_device_state.adc_config.prescaler = (reg1 >> 5) & 0x07;
    
    if (reg1 == 0) {
        g_device_state.adc_config.prescaler = 7;  /* Default: 128 */
    }
    
    hal_adc_set_prescaler(g_device_state.adc_config.prescaler);
}

void pixtend_core_set_mcu_config(uint8_t config)
{
    bool watchdog_was_enabled = g_device_state.mcu_config.watchdog_enabled;
    
    g_device_state.mcu_config.watchdog_enabled = (config & PIXTEND_UC_CTRL_WATCHDOG) != 0;
    
    if (g_device_state.mcu_config.watchdog_enabled && !watchdog_was_enabled) {
        hal_watchdog_enable(2000);  /* 2 second timeout */
    } else if (!g_device_state.mcu_config.watchdog_enabled && watchdog_was_enabled) {
        hal_watchdog_disable();
    }
}

/*=============================================================================
 * Output Control Functions
 *============================================================================*/

void pixtend_core_set_digital_outputs(uint8_t value)
{
    g_device_state.digital_outputs = value & 0x0F;
    hal_digital_outputs_write(g_device_state.digital_outputs);
}

void pixtend_core_set_relays(uint8_t value)
{
    g_device_state.relay_outputs = value & 0x0F;
    hal_relays_write(g_device_state.relay_outputs);
}

void pixtend_core_set_gpio_outputs(uint8_t value)
{
    /* Only update pins configured as outputs and not in DHT mode */
    uint8_t output_mask = g_device_state.gpio_config.direction & 
                         ~g_device_state.gpio_config.dht_enable;
    g_device_state.gpio_outputs = value & output_mask;
    hal_gpio_write_all(g_device_state.gpio_outputs);
}

void pixtend_core_set_pwm_value(uint8_t channel, uint8_t value_low, uint8_t value_high)
{
    uint16_t duty_cycle = (value_high << 8) | value_low;
    
    if (channel == 0 || channel == 2) {
        g_device_state.pwm_values[0] = duty_cycle;
        
        if (!g_device_state.pwm_config.is_pwm_mode) {
            /* Servo mode: apply overdrive if enabled */
            if (g_device_state.pwm_config.overdrive_ch0) {
                duty_cycle = 250 + duty_cycle - 128;
            } else {
                duty_cycle = 250 + value_low;
            }
        }
        hal_pwm_set_channel0(duty_cycle);
    }
    
    if (channel == 1 || channel == 2) {
        g_device_state.pwm_values[1] = duty_cycle;
        
        if (!g_device_state.pwm_config.is_pwm_mode) {
            /* Servo mode: apply overdrive if enabled */
            if (g_device_state.pwm_config.overdrive_ch1) {
                duty_cycle = 250 + duty_cycle - 128;
            } else {
                duty_cycle = 250 + value_low;
            }
        }
        hal_pwm_set_channel1(duty_cycle);
    }
}

/*=============================================================================
 * Input Acquisition Functions
 *============================================================================*/

void pixtend_core_read_digital_inputs(void)
{
    g_device_state.digital_inputs = hal_digital_inputs_read() & 0x0F;
}

void pixtend_core_read_gpio_inputs(void)
{
    g_device_state.gpio_inputs = hal_gpio_read_all() & 0x0F;
}

void pixtend_core_read_adc(uint8_t channel)
{
    if (channel >= PIXTEND_NUM_ADC_CHANNELS) {
        return;
    }
    
    uint8_t samples = get_adc_samples(g_device_state.adc_config.sample_rate[channel]);
    uint32_t sum = 0;
    
    for (uint8_t i = 0; i < samples; i++) {
        sum += hal_adc_read(channel);
    }
    
    uint16_t average = sum / samples;
    g_device_state.adc_values[channel].low = average & 0xFF;
    g_device_state.adc_values[channel].high = (average >> 8) & 0xFF;
}

void pixtend_core_read_dht(uint8_t channel)
{
    if (channel >= PIXTEND_NUM_DHT_CHANNELS) {
        return;
    }
    
    /* Check if DHT is enabled for this channel */
    if (!(g_device_state.gpio_config.dht_enable & (1 << (channel + PIXTEND_GPIO_DHT_ENABLE_SHIFT)))) {
        /* Sensor disabled - set values to 0 */
        g_device_state.humidity[channel].low = 0;
        g_device_state.humidity[channel].high = 0;
        g_device_state.temperature[channel].low = 0;
        g_device_state.temperature[channel].high = 0;
        return;
    }
    
    uint16_t humidity = 0, temperature = 0;
    uint8_t error = hal_dht_read(channel, &humidity, &temperature);
    
    if (error == PIXTEND_ERR_OK) {
        g_device_state.humidity[channel].low = humidity & 0xFF;
        g_device_state.humidity[channel].high = (humidity >> 8) & 0xFF;
        g_device_state.temperature[channel].low = temperature & 0xFF;
        g_device_state.temperature[channel].high = (temperature >> 8) & 0xFF;
    } else if (error == PIXTEND_ERR_DHT_DISABLED) {
        /* Sensor disabled */
        g_device_state.humidity[channel].low = 0;
        g_device_state.humidity[channel].high = 0;
        g_device_state.temperature[channel].low = 0;
        g_device_state.temperature[channel].high = 0;
    } else {
        /* Error - set values to 255 to indicate failure */
        g_device_state.humidity[channel].low = 255;
        g_device_state.humidity[channel].high = 255;
        g_device_state.temperature[channel].low = 255;
        g_device_state.temperature[channel].high = 255;
    }
}

void pixtend_core_read_digital_outputs(void)
{
    g_device_state.digital_outputs = hal_digital_outputs_read() & 0x0F;
}

void pixtend_core_read_relays(void)
{
    g_device_state.relay_outputs = hal_relays_read() & 0x0F;
}

/*=============================================================================
 * State Machine Management
 *============================================================================*/

bool pixtend_core_enter_run_state(void)
{
    if (!g_is_initialized) {
        return false;
    }
    
    g_device_state.mcu_config.is_initialized = true;
    pixtend_core_set_status_bit(PIXTEND_STATUS_INIT_STATE, true);
    return true;
}

bool pixtend_core_is_running(void)
{
    return g_device_state.mcu_config.is_initialized;
}

void pixtend_core_set_status_bit(uint8_t bit, bool value)
{
    if (bit > 7) {
        return;
    }
    
    if (value) {
        g_device_state.status_register |= (1 << bit);
    } else {
        g_device_state.status_register &= ~(1 << bit);
    }
}

uint8_t pixtend_core_get_status_register(void)
{
    return g_device_state.status_register;
}

/*=============================================================================
 * DHT Sensor Scheduling
 *============================================================================*/

uint8_t pixtend_core_update_dht_scheduler(void)
{
    uint8_t channel_to_poll = 255;  /* No polling by default */
    
    /* DHT sensors should be polled every 2 seconds */
    /* Automatic cycle time is 100ms, so poll every 20th cycle */
    switch (g_dht_scheduler_counter) {
        case 0:
            channel_to_poll = 0;
            break;
        case 5:
            channel_to_poll = 1;
            break;
        case 10:
            channel_to_poll = 2;
            break;
        case 15:
            channel_to_poll = 3;
            break;
        default:
            break;
    }
    
    g_dht_scheduler_counter++;
    if (g_dht_scheduler_counter > 19) {
        g_dht_scheduler_counter = 0;
    }
    
    return channel_to_poll;
}
