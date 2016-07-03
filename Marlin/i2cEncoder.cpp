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

#include "Marlin.h"
#include "i2cEncoder.h"
#include "temperature.h"
#include "stepper.h"

#include <Wire.h>

const char axis_codes[NUM_AXIS] = {'X', 'Y', 'Z', 'E'};

void I2cEncoder::init(AxisEnum axis, byte address) {
  set_axis(axis);
  set_address(address);
  initialised = true;
  SERIAL_ECHO("Encoder on ");
  SERIAL_ECHO(axis_codes[get_axis()]);
  SERIAL_ECHO(" axis, address = ");
  SERIAL_ECHOLN((int) address);
}

void I2cEncoder::update() {

  //check encoder is set up and active
  if(initialised && homed && active) {
    bool signalGood = passes_test(false);

    //check encoder data is good
    if(signalGood && trusted) {

      //get latest position
      position = get_position();

      //check error
      double error = get_axis_error_mm(false);

      #if defined(AXIS_ERROR_THRESHOLD_ABORT)
        if(error > AXIS_ERROR_THRESHOLD_ABORT) {
          kill("Significant Error");
        }
      #endif

      if(error > AXIS_ERROR_THRESHOLD_CORRECT) {
        babystepsTodo[encoderAxis] -= sgn(error) * STEPRATE;
      }

    } else {
      if(!signalGood) {
        lastErrorTime = millis();
        if(trusted) {
          trusted = false;
          SERIAL_ECHO("Error detected on ");
          SERIAL_ECHO(axis_codes[encoderAxis]);
          SERIAL_ECHO(" axis encoder. Disengaging error correction until module is trusted again.");
        }
      } else {

        //if the magnetic strength has been good for a certain time, start trusting the module again
        if(millis() - lastErrorTime > STABLE_TIME_UNTIL_TRUSTED) {
          trusted = true;

          //the encoder likely lost its place when the error occured, so we'll reset and use the printer's
          //idea of where it is to re-initialise

          //reset module's offset to zero (so current position is homed / zero)
          set_zeroed();

          //shift position from zero to current position
          zeroOffset = -(long) (st_get_axis_position_mm(encoderAxis) * ENCODER_TICKS_PER_MM);


          SERIAL_ECHO("Untrusted encoder module on ");
          SERIAL_ECHO(axis_codes[encoderAxis]);
          SERIAL_ECHO(" axis has been error-free for set duration, reinstating error correction.");
        }
      }
    } 
  }
}

void I2cEncoder::set_axis(AxisEnum axis) {
  encoderAxis = axis;
}

void I2cEncoder::set_address(byte address) {
  i2cAddress = address;
}

void I2cEncoder::set_homed() {
  if(active) {
    this->zeroOffset = get_raw_count();
    this->homed = true;
    this->trusted = true;
    #if defined(ENCODER_DEBUG_ECHOS)
      SERIAL_ECHO(axis_codes[encoderAxis]);
      SERIAL_ECHO(" axis encoder homed, offset of ");
      SERIAL_ECHO(zeroOffset);
      SERIAL_ECHOLN(" ticks.");
    #endif  
    }
}

bool I2cEncoder::passes_test(bool report) {
 byte magStrength = get_magnetic_strength();

  if(magStrength == I2C_MAG_SIG_BAD) {
    if(report) {
      SERIAL_ECHO("Warning, ");
      SERIAL_ECHO(axis_codes[encoderAxis]);
      SERIAL_ECHOLN(" axis magnetic strip not detected!");
    }
    return false; 
  } else if (magStrength == I2C_MAG_SIG_GOOD || magStrength == I2C_MAG_SIG_MID) { 
    if(report) {
      SERIAL_ECHO(axis_codes[encoderAxis]);
      SERIAL_ECHOLN(" axis encoder passes test.");
    }
    return true; 
  } else {
    if(report) {
      SERIAL_ECHO("Warning, ");
      SERIAL_ECHO(axis_codes[encoderAxis]);
      SERIAL_ECHOLN(" axis encoder not detected!");
    }
    return false;
  }
}

double I2cEncoder::get_axis_error_mm(bool report) {
  double target, actual, error;

  target = st_get_axis_position_mm(encoderAxis);
  actual = mmFromCount(position);
  error = actual - target;

  if(report) {
    SERIAL_ECHO(axis_codes[encoderAxis]);
    SERIAL_ECHO(" Target: ");
    SERIAL_ECHOLN(target);
    SERIAL_ECHO(axis_codes[encoderAxis]);
    SERIAL_ECHO(" Actual: ");
    SERIAL_ECHOLN(actual);
    SERIAL_ECHO(axis_codes[encoderAxis]);
    SERIAL_ECHO(" Error : ");
    SERIAL_ECHOLN(error);
  }

  return error;
}

double I2cEncoder::get_position_mm() {
  return mmFromCount(get_position());
}

double I2cEncoder::mmFromCount(long count) {
  return (double) count / ENCODER_TICKS_PER_MM;
}

long I2cEncoder::get_position() {
  return get_raw_count() - zeroOffset;
}

long I2cEncoder::get_raw_count() {
  i2cLong encoderCount;

  Wire.requestFrom((int)i2cAddress,4);

  byte index = 0;

  while (Wire.available()) {
    byte a = Wire.read();
    encoderCount.bval[index] = a;
    index += 1;
  }

  return encoderCount.val;

}

byte I2cEncoder::get_magnetic_strength() {

    //Set module to report magnetic strength
    Wire.beginTransmission((int)i2cAddress);
    Wire.write(I2C_ENC_REPORT_MODE_REGISTER);
    Wire.write(I2C_ENC_REPORT_MODE_STRENGTH);
    Wire.endTransmission();

    //Read value
    Wire.requestFrom((int)i2cAddress,1);

    byte reading = 99;

    reading = Wire.read();
    //SERIAL_ECHO(reading);

    //Set module back to normal (distance) mode
    Wire.beginTransmission((int)i2cAddress);
    Wire.write(I2C_ENC_REPORT_MODE_REGISTER);
    Wire.write(I2C_ENC_REPORT_MODE_DISTANCE);
    Wire.endTransmission();

    return reading;
  }

void I2cEncoder::set_zeroed() {
  //Set module to report magnetic strength
  Wire.beginTransmission(i2cAddress);
  Wire.write(1);
  Wire.endTransmission();
}

AxisEnum I2cEncoder::get_axis() {
  return encoderAxis;
}

bool I2cEncoder::get_active() {
  return active;
}

void I2cEncoder::set_active(bool a) {
  active = a;
}

EncoderManager::EncoderManager() {
  Wire.begin(); // We use no address so we will join the BUS as the master

}

void EncoderManager::init() {
  byte index = 0;

  #if defined(I2C_ENCODER_1_ADDR) && defined(I2C_ENCODER_1_AXIS)
    encoderArray[index].init(I2C_ENCODER_1_AXIS,I2C_ENCODER_1_ADDR);
    encoderArray[index].set_active(encoderArray[index].passes_test(true));
    index++;
  #endif  

  #if defined(I2C_ENCODER_2_ADDR) && defined(I2C_ENCODER_2_AXIS)
    encoderArray[index].init(I2C_ENCODER_2_AXIS,I2C_ENCODER_2_ADDR);
    encoderArray[index].set_active(encoderArray[index].passes_test(true));
    index++;
  #endif  

  #if defined(I2C_ENCODER_3_ADDR) && defined(I2C_ENCODER_3_AXIS)
    encoderArray[index].init(I2C_ENCODER_3_AXIS,I2C_ENCODER_3_ADDR);
    encoderArray[index].set_active(encoderArray[index].passes_test(true));
    index++;
  #endif  

  #if defined(I2C_ENCODER_4_ADDR) && defined(I2C_ENCODER_4_AXIS)
    encoderArray[index].init(I2C_ENCODER_4_AXIS,I2C_ENCODER_4_ADDR);
    encoderArray[index].set_active(encoderArray[index].passes_test(true));
    index++;
  #endif  

  #if defined(I2C_ENCODER_5_ADDR) && defined(I2C_ENCODER_5_AXIS)
    encoderArray[index].init(I2C_ENCODER_5_AXIS,I2C_ENCODER_5_ADDR);
    encoderArray[index].set_active(encoderArray[index].passes_test(true));
    index++;
  #endif  
}

void EncoderManager::update() {
  for(byte i = 0; i < NUM_AXIS; i++) {
    encoderArray[i].update(); 
  }
}

void EncoderManager::homed(AxisEnum axis) {
  for(byte i = 0; i < NUM_AXIS; i++) {
    if(encoderArray[i].get_axis() == axis) {
      encoderArray[i].set_homed();
    } 
  }
}