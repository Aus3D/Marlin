/**
 * Marlin 3D Printer Firmware
 *
 * Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 * Copyright (c) 2016 Bob Cousins bobcousins42@googlemail.com
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

#ifndef _HAL_TIMERS_STM32F4_H
#define _HAL_TIMERS_STM32F4_H

// --------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------

#include <stdint.h>

// --------------------------------------------------------------------------
// Defines
// --------------------------------------------------------------------------

/**
 * TODO: Check and confirm what timer we will use for each Temps and stepper driving.
 * We should probable drive temps with PWM.
 */
#define FORCE_INLINE __attribute__((always_inline)) inline

typedef uint32_t hal_timer_t; 		//TODO: One is 16-bit, one 32-bit - does this need to be checked?
#define HAL_TIMER_TYPE_MAX 0xFFFF	

#define STEP_TIMER_NUM 0  // index of timer to use for stepper
#define TEMP_TIMER_NUM 1  // index of timer to use for temperature

#define HAL_TIMER_RATE         (HAL_RCC_GetSysClockFreq()/2)  // frequency of timer peripherals
#define STEPPER_TIMER_PRESCALE 40            // prescaler for setting stepper timer, 2Mhz
#define HAL_STEPPER_TIMER_RATE (HAL_TIMER_RATE / STEPPER_TIMER_PRESCALE)   // frequency of stepper timer (HAL_TIMER_RATE / STEPPER_TIMER_PRESCALE)
#define HAL_TICKS_PER_US       ((HAL_STEPPER_TIMER_RATE) / 1000000) // stepper timer ticks per us

#define TEMP_TIMER_PRESCALE     1000 // prescaler for setting Temp timer, 80Khz
#define TEMP_TIMER_FREQUENCY    1000 // temperature interrupt frequency

#define STEP_TIMER_MIN_INTERVAL    8 // minimum time in µs between stepper interrupts

#define PULSE_TIMER_NUM STEP_TIMER_NUM
#define PULSE_TIMER_PRESCALE STEPPER_TIMER_PRESCALE

#define ENABLE_STEPPER_DRIVER_INTERRUPT() HAL_timer_enable_interrupt (STEP_TIMER_NUM)
#define DISABLE_STEPPER_DRIVER_INTERRUPT() HAL_timer_disable_interrupt (STEP_TIMER_NUM)
#define ENABLE_TEMPERATURE_INTERRUPT() HAL_timer_enable_interrupt (TEMP_TIMER_NUM)
#define DISABLE_TEMPERATURE_INTERRUPT() HAL_timer_disable_interrupt (TEMP_TIMER_NUM)
#define STEPPER_ISR_ENABLED() HAL_timer_interrupt_enabled(STEP_TIMER_NUM)

#define HAL_timer_get_count(timer_num) HAL_timer_get_current_count(timer_num)

#define HAL_ENABLE_ISRs() do { if (thermalManager.in_temp_isr)DISABLE_TEMPERATURE_INTERRUPT(); else ENABLE_TEMPERATURE_INTERRUPT(); ENABLE_STEPPER_DRIVER_INTERRUPT(); } while(0)
// TODO change this


extern void TC5_Handler();
extern void TC7_Handler();
#define HAL_STEP_TIMER_ISR  void TC5_Handler()
#define HAL_TEMP_TIMER_ISR  void TC7_Handler()

// --------------------------------------------------------------------------
// Types
// --------------------------------------------------------------------------

typedef struct {
  TIM_HandleTypeDef timerdef;
  IRQn_Type   IRQ_Id;
  uint32_t callback;
} tTimerConfig;

// --------------------------------------------------------------------------
// Public Variables
// --------------------------------------------------------------------------

//extern const tTimerConfig timerConfig[];

// --------------------------------------------------------------------------
// Public functions
// --------------------------------------------------------------------------

void HAL_timer_start (uint8_t timer_num, uint32_t frequency);
void HAL_timer_enable_interrupt(uint8_t timer_num);
void HAL_timer_disable_interrupt(uint8_t timer_num);
bool HAL_timer_interrupt_enabled(const uint8_t timer_num);

/**
 * NOTE: By default libmaple sets ARPE = 1, which means the Auto reload register is preloaded (will only update with an update event)
 * Thus we have to pause the timer, update the value, refresh, resume the timer.
 * That seems like a big waste of time and may be better to change the timer config to ARPE = 0, so ARR can be updated any time.
 * We are using a Channel in each timer in Capture/Compare mode. We could also instead use the Time Update Event Interrupt, but need to disable ARPE
 * so we can change the ARR value on the fly (without calling refresh), and not get an interrupt right there because we caused an UEV.
 * This mode pretty much makes 2 timers unusable for PWM since they have their counts updated all the time on ISRs.
 * The way Marlin manages timer interrupts doesn't make for an efficient usage in STM32F1
 * Todo: Look at that possibility later.
 */

void HAL_timer_set_compare(uint8_t timer_num, uint32_t count);
hal_timer_t HAL_timer_get_compare(uint8_t timer_num);
uint32_t HAL_timer_get_current_count(uint8_t timer_num);
void HAL_timer_isr_prologue (uint8_t timer_num);

FORCE_INLINE static void HAL_timer_restrain(const uint8_t timer_num, const uint16_t interval_ticks) {
  const hal_timer_t mincmp = HAL_timer_get_count(timer_num) + interval_ticks;
  if (HAL_timer_get_compare(timer_num) < mincmp) HAL_timer_set_compare(timer_num, mincmp);
}

#endif // _HAL_TIMERS_STM32F4_H
