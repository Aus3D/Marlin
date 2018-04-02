/**
 * Marlin 3D Printer Firmware
 *
 * Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 * Copyright (c) 2016 Bob Cousins bobcousins42@googlemail.com
 * Copyright (c) 2015-2016 Nico Tonnhofer wurstnase.reprap@gmail.com
 * Copyright (c) 2017 Victor Perez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * HAL for stm32duino.com based on Libmaple and compatible (STM32F1)
 */

#ifdef STM32F4

// --------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------

#include "../HAL.h"

// --------------------------------------------------------------------------
// Externals
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Local defines
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Types
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Variables
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Public Variables
// --------------------------------------------------------------------------

uint16_t HAL_adc_result;

// --------------------------------------------------------------------------
// Private Variables
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Function prototypes
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Private functions
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Public functions
// --------------------------------------------------------------------------

void HAL_clear_reset_source(void) { __HAL_RCC_CLEAR_RESET_FLAGS(); }

uint8_t HAL_get_reset_source (void) {
 if(__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
  return RST_WATCHDOG;

 if(__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != RESET)
   return RST_SOFTWARE;

 if(__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET)
   return RST_EXTERNAL;

 if(__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != RESET)
   return RST_POWER_ON;
  
  return 0;
}

void _delay_ms(const int delay_ms) { delay(delay_ms); }

extern "C" char* _sbrk(int incr);

int freeMemory() {
  volatile char top;
  return &top - reinterpret_cast<char*>(_sbrk(0));
}

// --------------------------------------------------------------------------
// ADC
// --------------------------------------------------------------------------

void HAL_adc_start_conversion(const uint8_t adc_pin) {
  HAL_adc_result = analogRead(adc_pin);
}

uint16_t HAL_adc_get_result(void) {
  return HAL_adc_result;
}

#endif // STM32F4
