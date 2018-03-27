/**
 * Marlin 3D Printer Firmware
 * Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 * Copyright (C) 2017 Victor Perez
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
 * Fast I/O interfaces for STM32F4
 * These use GPIO functions instead of Direct Port Manipulation, as on AVR.
 */

#ifndef	_FASTIO_STM32F4_H
#define	_FASTIO_STM32F4_H

#define READ(IO)              digitalRead(IO)
#define WRITE(IO, v)          digitalWrite(IO,v)
#define TOGGLE(IO)            digitalWrite(IO,!digitalRead(IO))
#define WRITE_VAR(IO, v)      digitalWrite(IO,v)

#define _GET_MODE(IO)         
#define _SET_MODE(IO,M)       pinMode(IO, M)
#define _SET_OUTPUT(IO)       pinMode(IO, OUTPUT)

#define SET_INPUT(IO)         _SET_MODE(IO, INPUT)
#define SET_INPUT_PULLUP(IO)  _SET_MODE(IO, INPUT_PULLUP)
#define SET_OUTPUT(IO)        do{ _SET_OUTPUT(IO); WRITE(IO, LOW); }while(0)

#define GET_INPUT(IO)         
#define GET_OUTPUT(IO)        
#define GET_TIMER(IO)         

#define OUT_WRITE(IO, v) { _SET_OUTPUT(IO); WRITE(IO, v); }

#endif	/* _FASTIO_STM32F4_H */
