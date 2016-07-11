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

#if defined(I2C_ENCODERS_ENABLED)

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

    bool moduleDetected;

    //we don't want to stop things just because the encoder missed a message,
    //so we only care about responses that indicate bad magnetic strength
    bool signalGood = passes_test(false,moduleDetected);

    //check encoder data is good
    if(signalGood) {

      //if data is historically good, proceed
      if(trusted) {

        //get latest position
        lastPosition = position;
        position = get_position();
        unsigned long positionTime = millis();

        #if defined(ERROR_THRESHOLD_PROPORTIONAL_SPEED)
          unsigned long distance = abs(position - lastPosition);
          unsigned long deltaTime = positionTime - lastPositionTime;
          unsigned long speed = distance / deltaTime;

          float threshold = constrain((speed / 50),1,50) * AXIS_ERROR_THRESHOLD_CORRECT;
        #else
          float threshold = AXIS_ERROR_THRESHOLD_CORRECT;
        #endif

        //check error
        double error = get_axis_error_mm(false);

        #if defined(AXIS_ERROR_THRESHOLD_ABORT)
          if(abs(error) > AXIS_ERROR_THRESHOLD_ABORT) {
            //kill("Significant Error");
            SERIAL_ECHO("Error, Kill! ");
            SERIAL_ECHOLN(error);
            delay(5000);
          }
        #endif



        if(abs(error) > threshold) {
          #if defined(ERROR_CORRECT_METHOD_1)

            thermalManager.babystepsTodo[encoderAxis] -= STEPRATE * sgn(error);

          #elif defined (ERROR_CORRECT_METHOD_2) 

            double axisPosition[NUM_AXIS];

            for(int i = 0; i < NUM_AXIS; i++) {
              axisPosition[i] = stepper.get_axis_position_mm((AxisEnum)i);
            }

            axisPosition[get_axis()] = mm_from_count(position);
            //stepper.set_position(get_axis(), lround(mm_from_count(position) * planner.axis_steps_per_mm[get_axis()]));
            planner.set_position_mm(axisPosition[X_AXIS],axisPosition[Y_AXIS],axisPosition[Z_AXIS],axisPosition[E_AXIS]);
            current_position[get_axis()] = mm_from_count(position);
            //SERIAL_ECHOLN("Correcting error");

          #endif

        }


        lastPositionTime = positionTime;
      } else {

        //if the magnetic strength has been good for a certain time, start trusting the module again
        if(millis() - lastErrorTime > STABLE_TIME_UNTIL_TRUSTED) {
          trusted = true;

          SERIAL_ECHO("Untrusted encoder module on ");
          SERIAL_ECHO(axis_codes[encoderAxis]);
          SERIAL_ECHOLN(" axis has been error-free for set duration, reinstating error correction.");

          //the encoder likely lost its place when the error occured, so we'll reset and use the printer's
          //idea of where it is to re-initialise
          double position = stepper.get_axis_position_mm(encoderAxis);
          long positionInTicks = position * ENCODER_TICKS_PER_MM;

          //shift position from zero to current position
          zeroOffset -= (positionInTicks - get_position());

          #if defined(ENCODER_DEBUG_ECHOS)
          
            SERIAL_ECHO("Current position is ");
            SERIAL_ECHOLN(position);

            SERIAL_ECHO("Position in encoder ticks is ");
            SERIAL_ECHOLN(positionInTicks);



            SERIAL_ECHO("New zero-offset of ");
            SERIAL_ECHOLN(zeroOffset);

            SERIAL_ECHO("New position reads as ");
            SERIAL_ECHO(get_position());
            SERIAL_ECHO("(");
            SERIAL_ECHO(mm_from_count(get_position()));
            SERIAL_ECHOLN(")");

          #endif
        }

      }

      

    } else {
      
      lastErrorTime = millis();
      if(trusted) {
        trusted = false;
        SERIAL_ECHO("Error detected on ");
        SERIAL_ECHO(axis_codes[encoderAxis]);
        SERIAL_ECHOLN(" axis encoder. Disengaging error correction until module is trusted again.");

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
    //reset module's offset to zero (so current position is homed / zero)
    set_zeroed();
    delay(10);
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
  bool encoderPresent;
  return (passes_test(report,encoderPresent) && encoderPresent);
}

bool I2cEncoder::passes_test(bool report, bool &moduleDetected) {
 byte magStrength = get_magnetic_strength();

  if(magStrength == I2C_MAG_SIG_BAD) {
    if(report) {
      SERIAL_ECHO("Warning, ");
      SERIAL_ECHO(axis_codes[encoderAxis]);
      SERIAL_ECHOLN(" axis magnetic strip not detected!");
    }
    moduleDetected = true;
    return false; 
  } else if (magStrength == I2C_MAG_SIG_GOOD || magStrength == I2C_MAG_SIG_MID) { 
    if(report) {
      SERIAL_ECHO(axis_codes[encoderAxis]);
      SERIAL_ECHOLN(" axis encoder passes test.");
    }
    moduleDetected = true;
    return true; 
  } else {
    if(report) {
      SERIAL_ECHO("Warning, ");
      SERIAL_ECHO(axis_codes[encoderAxis]);
      SERIAL_ECHOLN(" axis encoder not detected!");
    }
    moduleDetected = false;
    return true;
  }
}

double I2cEncoder::get_axis_error_mm(bool report) {
  double target, actual, error;

  target = stepper.get_axis_position_mm(encoderAxis);
  actual = mm_from_count(position);
  error = actual - target;

  if (abs(error) > 10000) {
    error = 0;
  }

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
  return mm_from_count(get_position());
}

double I2cEncoder::mm_from_count(long count) {
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

  //SERIAL_ECHOLN(encoderCount.val);

  if(invertDirection) {
    return -encoderCount.val;
  } else {
    return encoderCount.val;
  }

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
    //SERIAL_ECHO((int)reading);

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

bool I2cEncoder::get_inverted() {
  return invertDirection;
}

void I2cEncoder::set_inverted(bool inv) {
  invertDirection = inv;
}

EncoderManager::EncoderManager() {
  Wire.begin(); // We use no address so we will join the BUS as the master

}

void EncoderManager::init() {
  byte index = 0;

  #if defined(I2C_ENCODER_1_ADDR) && defined(I2C_ENCODER_1_AXIS)
    encoderArray[index].init(I2C_ENCODER_1_AXIS,I2C_ENCODER_1_ADDR);
    encoderArray[index].set_active(encoderArray[index].passes_test(true));
    #if defined(I2C_ENCODER_1_INVERT)
      encoderArray[index].set_inverted(true);
    #endif
    index++;
  #endif  

  #if defined(I2C_ENCODER_2_ADDR) && defined(I2C_ENCODER_2_AXIS)
    encoderArray[index].init(I2C_ENCODER_2_AXIS,I2C_ENCODER_2_ADDR);
    encoderArray[index].set_active(encoderArray[index].passes_test(true));
    #if defined(I2C_ENCODER_2_INVERT)
      encoderArray[index].set_inverted(true);
    #endif
    index++;
  #endif  

  #if defined(I2C_ENCODER_3_ADDR) && defined(I2C_ENCODER_3_AXIS)
    encoderArray[index].init(I2C_ENCODER_3_AXIS,I2C_ENCODER_3_ADDR);
    encoderArray[index].set_active(encoderArray[index].passes_test(true));
    #if defined(I2C_ENCODER_3_INVERT)
      encoderArray[index].set_inverted(true);
    #endif
    index++;
  #endif  

  #if defined(I2C_ENCODER_4_ADDR) && defined(I2C_ENCODER_4_AXIS)
    encoderArray[index].init(I2C_ENCODER_4_AXIS,I2C_ENCODER_4_ADDR);
    encoderArray[index].set_active(encoderArray[index].passes_test(true));
    #if defined(I2C_ENCODER_4_INVERT)
      encoderArray[index].set_inverted(true);
    #endif
    index++;
  #endif  

  #if defined(I2C_ENCODER_5_ADDR) && defined(I2C_ENCODER_5_AXIS)
    encoderArray[index].init(I2C_ENCODER_5_AXIS,I2C_ENCODER_5_ADDR);
    encoderArray[index].set_active(encoderArray[index].passes_test(true));
    #if defined(I2C_ENCODER_5_INVERT)
      encoderArray[index].set_inverted(true);
    #endif
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

void EncoderManager::report_position(AxisEnum axis, bool units, bool noOffset) {
  for(byte i = 0; i < NUM_AXIS; i++) {
    if(encoderArray[i].get_axis() == axis && encoderArray[i].get_active()) {
      if(units) {
        if(noOffset) {
          SERIAL_ECHOLN(encoderArray[i].mm_from_count(encoderArray[i].get_raw_count()));
        } else {
          SERIAL_ECHOLN(encoderArray[i].get_position_mm());
        }
      } else {
        if(noOffset) {
          SERIAL_ECHOLN(encoderArray[i].get_raw_count());
        } else {
          SERIAL_ECHOLN(encoderArray[i].get_position());
        }
      }
      break;
    } //else {
      //SERIAL_ECHO("Encoder not operational");
    //}
  }
}


void EncoderManager::report_status(AxisEnum axis) {
  bool responded = false;

  for(byte i = 0; i < NUM_AXIS; i++) {
    if(encoderArray[i].get_axis() == axis) {
      encoderArray[i].passes_test(true);    
      responded = true;
      break;
    //} else {
      //SERIAL_ECHO("Encoder not operational");
    }

    if (!responded) {
      SERIAL_ECHOLN("No encoder configured for given axis!");
      responded = true;
    }
  }
}

#endif


