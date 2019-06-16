/**
 * Marlin 3D Printer Firmware
 * Copyright (C) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
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

#ifndef STM32F4
  #error "Oops! Select an STM32F4 board in 'Tools > Board.'"
#endif

#define PICOPRINT_V1_0
#define DEFAULT_MACHINE_NAME "PICOPRINT"
#define BOARD_NAME "PICOPRINT"

//#define I2C_EEPROM

#define E2END 0xFFF // EEPROM end address (4kB)

#if HOTENDS > 2 || E_STEPPERS > 2
  #error "PICOPRINT supports up to 2 hotends / E-steppers."
#endif

//
// Limit Switches
//
#define X_MIN_PIN          PE10
#define X_MAX_PIN          -1
#define Y_MIN_PIN          PE9
#define Y_MAX_PIN          -1
#define Z_MIN_PIN          PE8
#define Z_MAX_PIN          -1

//
// Steppers
//
#define X_STEP_PIN         PC6
#define X_DIR_PIN          PC7
#define X_ENABLE_PIN       -1
#define X_CS_PIN           PC8

#define Y_STEP_PIN         PD9
#define Y_DIR_PIN          PD10
#define Y_ENABLE_PIN       -1
#define Y_CS_PIN           PD11

#define Z_STEP_PIN         PE15
#define Z_DIR_PIN          PB10
#define Z_ENABLE_PIN       -1
#define Z_CS_PIN           PD8

#define E0_STEP_PIN        PB1
#define E0_DIR_PIN         PB2
#define E0_ENABLE_PIN      -1
#define E0_CS_PIN          PE11

#define E1_STEP_PIN        PC4
#define E1_DIR_PIN         PC5
#define E1_ENABLE_PIN      -1
#define E1_CS_PIN          PB0

//
// Temperature Sensors
//
#define TEMP_0_PIN         PC0
#define TEMP_1_PIN         PC1
#define TEMP_2_PIN         PC2
#define TEMP_3_PIN         -1
#define TEMP_BED_PIN       PC3

//
// Heaters / Fans
//
#define HEATER_0_PIN       PD15
#define HEATER_1_PIN       PD14
#define HEATER_2_PIN       -1
#define HEATER_BED_PIN     PD12

#define FAN_PIN            PD13
#define FAN1_PIN           PA0
#define FAN2_PIN           PA1

//
// SPI
//
#define SCK_PIN            PE12
#define MISO_PIN           PE13
#define MOSI_PIN           PE14

//
// Misc. Functions
//
#define LED_PIN            PB14
#define BTN_PIN            PC10
#define PS_ON_PIN          -1
#define KILL_PIN           -1

#define SDSS               PB15
#define SD_DETECT_PIN      PA8
#define BEEPER_PIN         PC9

//
// LCD / Controller
//
#define LCD_PINS_RS        PC12
#define LCD_PINS_ENABLE    PD7
#define LCD_PINS_D4        PD1
#define LCD_PINS_D5        PD3
#define LCD_PINS_D6        PD2
#define LCD_PINS_D7        PD4
#define BTN_EN1            PD6
#define BTN_EN2            PD0
#define BTN_ENC            PC11
