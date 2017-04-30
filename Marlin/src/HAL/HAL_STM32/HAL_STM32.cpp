/* **************************************************************************
 
 Marlin 3D Printer Firmware
 Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 Copyright (c) 2016 Bob Cousins bobcousins42@googlemail.com
   
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


/**
 * Description: HAL for Arduino Due and compatible (SAM3X8E)
 *
 * For ARDUINO_ARCH_SAM
 */

#ifdef STM32F4

// --------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------

#include "../HAL.h"

#include <Wire.h>
#include "STM32_HAL.h"

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

// disable interrupts
void cli(void) { __disable_irq(); }

// enable interrupts
void sei(void) { __enable_irq(); }

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

void _delay_ms(int delay_ms) {
	//todo: port for Due?
	delay (delay_ms);
}

extern "C" {
  extern unsigned int _ebss; // end of bss section
}

// return free memory between end of heap (or end bss) and whatever is current
int freeMemory() {
  //int free_memory, heap_end = (int)_sbrk(0);
  return 1000;//(int)&free_memory - (heap_end ? heap_end : (int)&_ebss);
}

// --------------------------------------------------------------------------
// ADC
// --------------------------------------------------------------------------

void HAL_adc_start_conversion (uint8_t adc_pin) {
	HAL_adc_result = analogRead(adc_pin);
}

uint16_t HAL_adc_get_result(void) {
	// nop
	return HAL_adc_result;
}

#endif // STM32F4
