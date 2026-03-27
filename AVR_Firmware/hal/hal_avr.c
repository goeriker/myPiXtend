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
 * @file hal_avr.c
 * @brief Hardware Abstraction Layer implementation for ATmega32A
 */

#include "hal.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <util/crc16.h>

/*=============================================================================
 * GPIO HAL Implementation
 *============================================================================*/

void hal_gpio_init(void)
{
    /* GPIO pins PA0-PA3: initially inputs */
    DDRA &= ~0x0F;
    PORTA &= ~0x0F;  /* No pull-ups */
}

void hal_gpio_set_direction(uint8_t pin, bool is_output)
{
    if (pin > 3) return;
    
    if (is_output) {
        DDRA |= (1 << pin);
    } else {
        DDRA &= ~(1 << pin);
    }
}

void hal_gpio_write(uint8_t pin, bool value)
{
    if (pin > 3) return;
    
    if (value) {
        PORTA |= (1 << pin);
    } else {
        PORTA &= ~(1 << pin);
    }
}

bool hal_gpio_read(uint8_t pin)
{
    if (pin > 3) return false;
    return (PINA & (1 << pin)) != 0;
}

uint8_t hal_gpio_read_all(void)
{
    return PINA & 0x0F;
}

void hal_gpio_write_all(uint8_t value)
{
    PORTA = (PORTA & 0xF0) | (value & 0x0F);
}

/*=============================================================================
 * Digital I/O HAL Implementation
 *============================================================================*/

void hal_dio_init(void)
{
    /* Digital inputs: PD0-PD3, PB4-PB7 */
    DDRD &= ~0x0F;
    DDRB &= ~0xF0;
    PORTD &= ~0x0F;  /* No pull-ups */
    PORTB &= ~0xF0;
    
    /* Digital outputs: PC0-PC3, PD6-PD7 */
    DDRC |= 0x0F;
    DDRD |= 0xC0;
    PORTC &= ~0x0F;
    PORTD &= ~0xC0;
    
    /* Relay outputs: PC4-PC7 */
    DDRC |= 0xF0;
    PORTC &= ~0xF0;
}

uint8_t hal_digital_inputs_read(void)
{
    return ((PIND & 0x0F) | ((PINB & 0xF0) >> 4)) & 0x0F;
}

void hal_digital_outputs_write(uint8_t value)
{
    PORTC = (PORTC & 0xF0) | (value & 0x0F);
    uint8_t temp = ((value << 3) & 0x80) | ((value << 1) & 0x40);
    PORTD = (PORTD & 0x3F) | temp;
}

uint8_t hal_digital_outputs_read(void)
{
    return ((PINC & 0x0F) | ((PIND & 0x80) >> 3) | ((PIND & 0x40) >> 1)) & 0x0F;
}

void hal_relays_write(uint8_t value)
{
    PORTC = (PORTC & 0x0F) | ((value << 4) & 0xF0);
}

uint8_t hal_relays_read(void)
{
    return (PINC >> 4) & 0x0F;
}

/*=============================================================================
 * ADC HAL Implementation
 *============================================================================*/

void hal_adc_init(void)
{
    /* External reference at AREF */
    ADMUX = (1 << MUX2);  /* Start with ADC4 (ANALOG_IN 0) */
    ADCSRA = (1 << ADEN);  /* Enable ADC */
    ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);  /* Prescaler 128 */
    ADCSRA |= (1 << ADSC);  /* Start first conversion */
}

void hal_adc_set_prescaler(uint8_t prescaler)
{
    ADCSRA = (ADCSRA & 0xF8) | (prescaler & 0x07);
}

uint16_t hal_adc_read(uint8_t channel)
{
    if (channel > 3) return 0;
    
    /* Set channel, keep other bits */
    ADMUX = (ADMUX & 0xE0) | ((channel + 4) & 0x1F);
    
    /* Start conversion */
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {
        ;  /* Wait for completion */
    }
    
    return ADC;
}

/*=============================================================================
 * PWM/Servo HAL Implementation
 *============================================================================*/

void hal_pwm_init(void)
{
    /* Timer 1: Fast PWM mode, TOP = ICR1 */
    TCCR1B = (1 << WGM13) | (1 << WGM12);
    TCCR1A = (1 << WGM11);
    
    /* Clear OC1A/OC1B on compare match */
    TCCR1A |= (1 << COM1A1) | (1 << COM1B1);
    
    /* Prescaler 64 */
    TCCR1B |= (1 << CS11) | (1 << CS10);
    
    /* Default: Servo mode, 20ms period */
    ICR1 = 5000;
    OCR1A = 250;  /* 1ms (servo 0°) */
    OCR1B = 250;
    
    /* PWM pins: PD4 (OC1A), PD5 (OC1B) as outputs */
    DDRD |= 0x30;
}

void hal_pwm_configure(uint8_t prescaler, uint16_t top_value, bool is_pwm_mode)
{
    /* Update prescaler */
    TCCR1B = (TCCR1B & 0xF8) | (prescaler & 0x07);
    ICR1 = top_value;
    
    if (!is_pwm_mode) {
        /* Servo mode: reset to default positions */
        OCR1A = 250;
        OCR1B = 250;
    }
}

void hal_pwm_set_channel0(uint16_t duty_cycle)
{
    OCR1A = duty_cycle;
}

void hal_pwm_set_channel1(uint16_t duty_cycle)
{
    OCR1B = duty_cycle;
}

/*=============================================================================
 * DHT Sensor HAL Implementation
 *============================================================================*/

void hal_dht_init(void)
{
    /* DHT sensors use GPIO pins PA0-PA3 */
    /* Initialization is done per-channel via hal_dht_enable */
}

void hal_dht_enable(uint8_t channel, bool enabled)
{
    if (channel > 3) return;
    
    if (enabled) {
        /* Pin will be controlled by DHT read function */
        /* Initially set to input with no pull-up */
        DDRA &= ~(1 << channel);
        PORTA &= ~(1 << channel);
    }
}

uint8_t hal_dht_read(uint8_t channel, uint16_t* humidity, uint16_t* temperature)
{
    if (channel > 3) return 1;
    
    uint8_t timeout = 0;
    uint8_t dht_data[5] = {0};
    uint8_t dht_byte = 0;
    
    /* Check if bus is free */
    DDRA &= ~(1 << channel);  /* Set to input */
    _delay_us(10);
    
    if (!(PINA & (1 << channel))) {
        return PIXTEND_ERR_DHT_BUSY;
    }
    
    /* Send start signal */
    DDRA |= (1 << channel);  /* Output */
    PORTA &= ~(1 << channel);  /* Low */
    _delay_ms(20);  /* Start signal (20ms) */
    DDRA &= ~(1 << channel);  /* Input again */
    
    /* Wait for sensor response (low) */
    timeout = 185;
    _delay_us(15);
    while (PINA & (1 << channel)) {
        _delay_us(1);
        if (!timeout--) {
            return PIXTEND_ERR_DHT_TIMEOUT_START;
        }
    }
    
    /* Response low time (80us) */
    timeout = 70;
    _delay_us(15);
    while (!(PINA & (1 << channel))) {
        _delay_us(1);
        if (!timeout--) {
            return PIXTEND_ERR_DHT_TIMEOUT_RESP_L;
        }
    }
    
    /* Response high time (80us) */
    timeout = 70;
    _delay_us(15);
    while (PINA & (1 << channel)) {
        _delay_us(1);
        if (!timeout--) {
            return PIXTEND_ERR_DHT_TIMEOUT_RESP_H;
        }
    }
    
    /* Read 40 bits of data */
    for (uint8_t i = 0; i < 5; i++) {
        dht_byte = 0;
        for (uint8_t j = 0; j < 8; j++) {
            /* Wait for bit start (low) */
            timeout = 55;
            while (!(PINA & (1 << channel))) {
                _delay_us(1);
                if (!timeout--) {
                    return PIXTEND_ERR_DHT_TIMEOUT_BIT_L;
                }
            }
            
            _delay_us(30);  /* Wait to sample */
            dht_byte <<= 1;
            
            if (PINA & (1 << channel)) {
                /* Bit is 1 */
                dht_byte |= 1;
                timeout = 45;
                while (PINA & (1 << channel)) {
                    _delay_us(1);
                    if (!timeout--) {
                        return PIXTEND_ERR_DHT_TIMEOUT_BIT_H;
                    }
                }
            }
        }
        dht_data[i] = dht_byte;
    }
    
    /* Verify checksum */
    if (((dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) & 0xFF) != dht_data[4]) {
        return PIXTEND_ERR_DHT_CHECKSUM;
    }
    
    /* Store values */
    *humidity = (dht_data[0] << 8) | dht_data[1];
    *temperature = (dht_data[2] << 8) | dht_data[3];
    
    return PIXTEND_ERR_OK;
}

/*=============================================================================
 * Watchdog HAL Implementation
 *============================================================================*/

void hal_watchdog_init(void)
{
    /* Watchdog is enabled/disabled as needed */
}

void hal_watchdog_enable(uint16_t timeout_ms)
{
    if (timeout_ms <= 15) {
        wdt_enable(WDTO_15MS);
    } else if (timeout_ms <= 30) {
        wdt_enable(WDTO_30MS);
    } else if (timeout_ms <= 60) {
        wdt_enable(WDTO_60MS);
    } else if (timeout_ms <= 120) {
        wdt_enable(WDTO_120MS);
    } else if (timeout_ms <= 250) {
        wdt_enable(WDTO_250MS);
    } else if (timeout_ms <= 500) {
        wdt_enable(WDTO_500MS);
    } else if (timeout_ms <= 1000) {
        wdt_enable(WDTO_1S);
    } else if (timeout_ms <= 2000) {
        wdt_enable(WDTO_2S);
    } else {
        wdt_enable(WDTO_2S);
    }
}

void hal_watchdog_disable(void)
{
    wdt_disable();
}

void hal_watchdog_reset(void)
{
    wdt_reset();
}

/*=============================================================================
 * SPI HAL Implementation
 *============================================================================*/

void hal_spi_init(void)
{
    /* SPI pins: PB4 (MISO), PB5 (MOSI), PB6 (SCK), PB7 (SS) */
    /* SS must be output for master mode, but we're slave */
    DDRB &= ~((1 << PB4) | (1 << PB5) | (1 << PB6));  /* MISO, MOSI, SCK inputs */
    DDRB |= (1 << PB7);  /* SS can be input or output */
    PORTB &= ~((1 << PB4) | (1 << PB5) | (1 << PB6) | (1 << PB7));
}

void hal_spi_enable_interrupt(void)
{
    SPCR |= (1 << SPIE);
}

void hal_spi_disable_interrupt(void)
{
    SPCR &= ~(1 << SPIE);
}

uint8_t hal_spi_get_data(void)
{
    return SPDR;
}

void hal_spi_put_data(uint8_t data)
{
    SPDR = data;
}

/*=============================================================================
 * System HAL Implementation
 *============================================================================*/

void hal_system_init(void)
{
    /* System clock is configured by fuse bits */
    /* No additional initialization needed */
}

void hal_interrupts_enable(void)
{
    sei();
}

void hal_interrupts_disable(void)
{
    cli();
}

void hal_delay_us(uint16_t us)
{
    _delay_us(us);
}

void hal_delay_ms(uint16_t ms)
{
    _delay_ms(ms);
}

uint16_t hal_crc16_update(uint16_t crc, uint8_t data)
{
    return _crc16_update(crc, data);
}
