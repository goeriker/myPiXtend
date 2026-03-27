# PiXtend Firmware Refactored - Build Instructions

## Overview

This refactored firmware separates business logic from hardware-specific code, making it portable to different MCU families. The architecture follows a layered approach with clear separation of concerns.

## Directory Structure

```
AVR_Firmware/
├── core/                        # Hardware-independent business logic
│   ├── pixtend_types.h          # Type definitions and constants
│   ├── pixtend_core.h/.c        # Device state management & low-level ops
│   ├── pixtend_automatic_mode.h/.c  # Automatic mode SPI protocol
│   └── pixtend_manual_mode.h/.c     # Manual mode command handling
├── hal/                         # Hardware Abstraction Layer
│   ├── hal.h                    # HAL interface definitions
│   └── hal_avr.c                # AVR/ATmega32A implementation
├── drivers/                     # Device drivers (future expansion)
├── modes/                       # Mode-specific implementations (future)
├── main_new.c                   # New main entry point
├── main.c                       # Original monolithic code (backup)
└── Makefile                     # Build configuration
```

## Architecture

### Layer 1: Core Module (`core/`)

The core module is divided into three components:

#### `pixtend_types.h`
- Common type definitions (`PixtendDeviceState_t`, `PixtendAdcValue_t`, etc.)
- Hardware configuration constants (number of channels, etc.)
- SPI communication constants (commands, bit definitions)
- Error codes

#### `pixtend_core.h/c`
- **Hardware-independent** device state management
- Configuration update functions (GPIO, PWM, ADC, MCU)
- Output control functions (digital outputs, relays, GPIO, PWM)
- Input acquisition functions (digital inputs, ADC, DHT)
- State machine management (INIT → RUN transition)

#### `pixtend_automatic_mode.h/c`
- Automatic mode SPI protocol processing
- CRC validation
- DHT sensor scheduling (polling every 2 seconds)
- Response buffer preparation
- Statistics tracking (successful cycles, CRC errors)

#### `pixtend_manual_mode.h/c`
- Manual mode command interpreter
- Complete command set implementation (read/write registers)
- Multi-byte command state machine
- Statistics tracking (commands processed, errors)

### Layer 2: HAL Interface (`hal/hal.h`)

Abstract interface definitions that must be implemented for each target platform:

- **GPIO**: Pin direction, read/write operations
- **Digital I/O**: Digital inputs/outputs, relays
- **ADC**: Channel selection, prescaler, reading
- **PWM/Servo**: Configuration, duty cycle setting
- **DHT Sensor**: Enable, read with timeouts
- **Watchdog**: Enable/disable/reset
- **SPI**: Data register access, interrupts
- **System**: Initialization, delays, CRC, interrupts

### Layer 3: HAL Implementation (`hal/hal_*.c`)

Platform-specific implementations:

- Current: `hal_avr.c` for ATmega32A
- Future candidates:
  - `hal_stm32.c` for STM32F1/F4
  - `hal_esp32.c` for ESP32
  - `hal_rp2040.c` for Raspberry Pi Pico

### Layer 4: Application (`main_new.c`)

Minimal main entry point (~200 lines):
- Hardware initialization via HAL
- SPI interrupt handler
- Main event loop (processes automatic mode cycles)
- Watchdog management

## Porting to Another MCU Family

To port this firmware to a different MCU (e.g., STM32, ESP32):

### Step 1: Create New HAL Implementation

Create `hal/hal_<platform>.c` implementing all functions from `hal/hal.h`:

```c
// Example: hal_stm32.c
#include "hal.h"
#include <stm32f4xx.h>  // Platform-specific headers

void hal_gpio_init(void) {
    // Enable GPIO clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    // Configure pins for PiXtend
    // ...
}

void hal_adc_read(uint8_t channel) {
    // STM32-specific ADC conversion
    // ...
}

// ... implement all other HAL functions
```

### Step 2: Update Build Configuration

Modify Makefile to:
1. Replace `hal_avr.c` with `hal_<platform>.c`
2. Set correct compiler flags for target platform
3. Link against platform-specific libraries
4. Adjust memory layout (flash, RAM sizes)

### Step 3: Platform-Specific Main (if needed)

If the platform requires special initialization:

```c
// main_stm32.c
#include "hal/hal.h"
#include "core/pixtend_core.h"
#include "core/pixtend_automatic_mode.h"
#include "core/pixtend_manual_mode.h"

int main(void) {
    // STM32 system initialization
    SystemInit();
    SysTick_Config(SystemCoreClock / 1000);  // 1ms tick
    
    // Standard initialization
    hal_system_init();
    hal_gpio_init();
    hal_dio_init();
    // ... rest is identical to AVR version
}
```

### Step 4: Verify SPI Protocol

Ensure the SPI slave implementation matches the Raspberry Pi's expectations:
- SPI mode (CPOL, CPHA)
- Bit order (MSB first)
- Interrupt-driven data exchange

## Key Benefits

1. **Separation of Concerns**: Business logic completely separated from hardware
2. **Testability**: Core logic can be unit-tested on host machine with mock HAL
3. **Maintainability**: Clear boundaries between layers, focused modules
4. **Portability**: Only HAL needs reimplementation for new platforms
5. **Readability**: Well-documented interfaces, modular structure
6. **Extensibility**: Easy to add new features without touching hardware code
7. **Statistics**: Built-in monitoring for both automatic and manual modes

## Building for AVR (ATmega32A)

```bash
cd AVR_Firmware
make
```

This produces:
- `pixtend.hex` - Intel HEX file for programming
- `pixtend.lst` - Disassembly listing
- `pixtend.map` - Memory map

## API Reference

### Automatic Mode Commands

See `core/pixtend_types.h`:
- `PIXTEND_SPI_CMD_AUTO_MODE` (0x80) - Enter automatic mode
- Transfer size: 31 bytes (29 data + 2 CRC)

### Manual Mode Commands

See `core/pixtend_types.h`:

**Write Commands:**
- `PIXTEND_CMD_WRITE_DO` (0x01) - Digital outputs
- `PIXTEND_CMD_WRITE_RELAYS` (0x07) - Relay outputs
- `PIXTEND_CMD_WRITE_SERVO0/1` (0x80/0x81) - Servo positions
- `PIXTEND_CMD_WRITE_PWM0/1` (0x82/0x83) - PWM duty cycles
- `PIXTEND_CMD_WRITE_PWM_CTRL` (0x84) - PWM configuration
- `PIXTEND_CMD_WRITE_GPIO_CTRL` (0x85) - GPIO configuration
- `PIXTEND_CMD_WRITE_UC_CTRL` (0x86) - MCU control

**Read Commands:**
- `PIXTEND_CMD_READ_DI` (0x02) - Digital inputs
- `PIXTEND_CMD_READ_ADC0-3` (0x03-0x06) - ADC values
- `PIXTEND_CMD_READ_TEMP0-3` (0x0A-0x0D) - Temperature
- `PIXTEND_CMD_READ_HUMID0-3` (0x0E-0x11) - Humidity
- `PIXTEND_CMD_READ_VERSION` (0x89) - Firmware version
- `PIXTEND_CMD_READ_UC_STATUS` (0x8A) - MCU status

## Migration Notes

The original `main.c` (1573 lines) has been refactored into:

| Component | Lines | Description |
|-----------|-------|-------------|
| `pixtend_types.h` | 231 | Type definitions |
| `pixtend_core.h/c` | 902 | Device state management |
| `pixtend_automatic_mode.h/c` | 329 | Automatic mode protocol |
| `pixtend_manual_mode.h/c` | 349 | Manual mode commands |
| `hal.h/hal_avr.c` | 738 | Hardware abstraction |
| `main_new.c` | 203 | Application entry point |
| **Total** | **2752** | Modular, maintainable code |

While total line count increased (~75%), this is due to:
- Comprehensive documentation
- Clear interface definitions
- Separated concerns (no monolithic functions)
- Statistics and error tracking

The new structure is significantly more maintainable and portable.

## Testing Recommendations

1. **Unit Tests**: Test core logic with mock HAL on PC
2. **Integration Tests**: Test with actual hardware
3. **Protocol Tests**: Verify SPI communication with Raspberry Pi
4. **Stress Tests**: Long-running automatic mode operation
5. **Porting Tests**: Verify behavior consistency across platforms

## License

Same as original: GNU General Public License v3 or later.
