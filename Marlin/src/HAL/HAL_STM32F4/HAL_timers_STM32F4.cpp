/**
 * Marlin 3D Printer Firmware
 *
 * Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 * Copyright (c) 2016 Bob Cousins bobcousins42@googlemail.com
 * Copyright (c) 2015-2016 Nico Tonnhofer wurstnase.reprap@gmail.com
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
#include "HAL_timers_STM32F4.h"

// --------------------------------------------------------------------------
// Externals
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Local defines
// --------------------------------------------------------------------------

#define NUM_HARDWARE_TIMERS 2

// --------------------------------------------------------------------------
// Types
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Public Variables
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Private Variables
// --------------------------------------------------------------------------

tTimerConfig timerConfig[NUM_HARDWARE_TIMERS];
bool timers_initialised[NUM_HARDWARE_TIMERS] = {false};

// --------------------------------------------------------------------------
// Function prototypes
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Private functions
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Public functions
// --------------------------------------------------------------------------

void HAL_timer_start(uint8_t timer_num, uint32_t frequency) {
  if(!timers_initialised[timer_num]) {
    switch (timer_num) {
      case STEP_TIMER_NUM:
      //STEPPER TIMER TIM5 //use a 32bit timer 
      __HAL_RCC_TIM5_CLK_ENABLE();
      timerConfig[STEP_TIMER_NUM].timerdef.Instance               = STEP_TIMER;
      timerConfig[STEP_TIMER_NUM].timerdef.Init.Prescaler         = (STEPPER_TIMER_PRESCALE);
      timerConfig[STEP_TIMER_NUM].timerdef.Init.CounterMode       = TIM_COUNTERMODE_UP;
      timerConfig[STEP_TIMER_NUM].timerdef.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
      timerConfig[STEP_TIMER_NUM].IRQ_Id                          = TIM5_IRQn;
      HAL_NVIC_SetPriority(timerConfig[STEP_TIMER_NUM].IRQ_Id, 1, 0);
      pinMode(STEPPER_ENABLE_PIN,OUTPUT);
      digitalWrite(STEPPER_ENABLE_PIN,LOW);
      break;
    case TEMP_TIMER_NUM:
      //TEMP TIMER TIM7 // any available 16bit Timer (1 already used for PWM)
      __HAL_RCC_TIM7_CLK_ENABLE();
      timerConfig[TEMP_TIMER_NUM].timerdef.Instance               = TEMP_TIMER;
      timerConfig[TEMP_TIMER_NUM].timerdef.Init.Prescaler         = (TEMP_TIMER_PRESCALE); 
      timerConfig[TEMP_TIMER_NUM].timerdef.Init.CounterMode       = TIM_COUNTERMODE_UP;   
      timerConfig[TEMP_TIMER_NUM].timerdef.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
      timerConfig[TEMP_TIMER_NUM].IRQ_Id                          = TIM7_IRQn;
      HAL_NVIC_SetPriority(timerConfig[TEMP_TIMER_NUM].IRQ_Id, 2, 0);
      break;
    }
    timers_initialised[timer_num] = true;
  }

  timerConfig[timer_num].timerdef.Init.Period = ((HAL_TIMER_RATE / timerConfig[timer_num].timerdef.Init.Prescaler) / (frequency)) - 1;

  if(HAL_TIM_Base_Init(&timerConfig[timer_num].timerdef) == HAL_OK) {
    HAL_TIM_Base_Start_IT(&timerConfig[timer_num].timerdef);
  } 
}

void HAL_timer_enable_interrupt(uint8_t timer_num) {
  HAL_NVIC_EnableIRQ(timerConfig[timer_num].IRQ_Id);
}

void HAL_timer_disable_interrupt(uint8_t timer_num) {
  HAL_NVIC_DisableIRQ(timerConfig[timer_num].IRQ_Id);
}

void HAL_timer_isr_prologue(uint8_t timer_num) {
  if (__HAL_TIM_GET_FLAG(&timerConfig[timer_num].timerdef, TIM_FLAG_UPDATE) == SET) {
    __HAL_TIM_CLEAR_FLAG(&timerConfig[timer_num].timerdef, TIM_FLAG_UPDATE);
  }
}

bool HAL_timer_interrupt_enabled(const uint8_t timer_num) {
  switch (timer_num) {
    case STEP_TIMER_NUM: return HAL_NVIC_GetActive(timerConfig[timer_num].IRQ_Id);
    case TEMP_TIMER_NUM: return HAL_NVIC_GetActive(timerConfig[timer_num].IRQ_Id);
  }
  return false;
}

#endif // __STM32F4__
