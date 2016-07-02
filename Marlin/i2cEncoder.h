/*
 * Marlin 3D Printer Firmware
 * Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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

//===========================================================================
//============================ I2C Encoder Settings =========================
//===========================================================================

//this section should be moved to Configuration.h at some stage
#define I2C_ENCODERS_ENABLED

#if defined(I2C_ENCODERS_ENABLED)

//Enable and configure encoders
#define I2C_ENCODER_1_ADDR I2C_ENCODER_PRESET_ADDR_X
#define I2C_ENCODER_1_AXIS X_AXIS

#define I2C_ENCODER_2_ADDR I2C_ENCODER_PRESET_ADDR_Y
#define I2C_ENCODER_2_AXIS Y_AXIS

#define I2C_ENCODER_3_ADDR I2C_ENCODER_PRESET_ADDR_Z
#define I2C_ENCODER_3_AXIS Z_AXIS

#define ENCODER_TICKS_PER_MM 2048


//Configure error correction
#define AXIS_ERROR_THRESHOLD_ABORT 100.0  //number of mm error in any given axis after which the printer will abort. Comment out to disable abort behaviour.
#define AXIS_ERROR_THRESHOLD_CORRECT 0.050    //number of mm in error above which the printer will attempt to correct the error, errors smaller than this are ignored to avoid measurement noise / latency (filter)
#define STABLE_TIME_UNTIL_TRUSTED 10000 //after an error, there must be no errors for this period (ms) before the encoder is trusted again

#define ERROR_CORRECT_METHOD 1
#define STEPRATE 1

#endif








#if defined(I2C_ENCODERS_ENABLED)

#ifndef I2CENC_H
#define I2CENC_H

#include "macros.h"

#define BABYSTEPPING    //babysteppping needs to be enabled for error correction to work

//I2C defines / enums etc
#define I2C_MAG_SIG_GOOD 0
#define I2C_MAG_SIG_MID 1
#define I2C_MAG_SIG_BAD 2

#define I2C_ENC_LED_PAR_MODE 10
#define I2C_ENC_LED_PAR_BRT 11
#define I2C_ENC_LED_PAR_RATE 14

//default I2C addresses
#define I2C_ENCODER_PRESET_ADDR_X 30
#define I2C_ENCODER_PRESET_ADDR_Y 31
#define I2C_ENCODER_PRESET_ADDR_Z 32
#define I2C_ENCODER_PRESET_ADDR_E 33

#define I2C_ENCODER_DEF_AXIS X_AXIS
#define I2C_ENCODER_DEF_ADDR I2C_ENCODER_PRESET_ADDR_X

typedef union{
  volatile long val = 0;
  byte bval[4];
}i2cLong;

void gcode_M860();
void gcode_M861();
void gcode_M862();
void gcode_M863();
void gcode_M865();
void gcode_M866();
void gcode_M867();

void i2c_encoder_init();
void check_axis_errors();
void correct_axis_errors(bool);
void set_encoder_homed(AxisEnum);
void report_encoder_positions(AxisEnum);
void report_encoder_positions_mm(AxisEnum);

void calculate_axis_steps_per_unit(AxisEnum, int);

const char axis_codes[NUM_AXIS] = {'X', 'Y', 'Z', 'E'};

class I2cEncoder {
    private:

        byte i2cAddress = I2C_ENCODER_DEF_ADDR;
        long zeroOffset = 0;
        bool homed = false;
        bool trusted = false;
        bool initialised = false;
        long position;
        double positionMm;
        unsigned long lastErrorTime;
        AxisEnum encoderAxis = I2C_ENCODER_DEF_AXIS;

        double mmFromCount(long count);

    public:

        void init(AxisEnum axis, byte address);
        void update();
        void set_homed();
        double get_axis_error_mm(bool report);
        double get_position_mm();
        long get_position();
        long get_raw_count();
        void set_led_param(byte, byte, byte);
        void set_zeroed();
        bool passes_test(bool report);
        byte get_magnetic_strength();

        void set_axis(AxisEnum axis);
        void set_address(byte address);
};

class EncoderManager {
    private:
        I2cEncoder encoderArray[NUM_AXIS];

    public:
        EncoderManager();
        void init();
        void update();

};





int get_encoder_axis_address(AxisEnum);

static inline int8_t sgn(int val) {
 if (val < 0) return -1;
 if (val==0) return 0;
 return 1;
}

#endif //I2CENC_H
#endif //I2C_ENCODERS_ENABLED


