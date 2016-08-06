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

#if defined(I2C_ENCODERS_ENABLED)

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
  if(initialised && homed && active && errorCorrect) {

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
            planner.set_position_mm(axisPosition[X_AXIS],axisPosition[Y_AXIS],axisPosition[Z_AXIS],axisPosition[E_AXIS]);
            current_position[get_axis()] = mm_from_count(position);

          #endif

          if(abs(error) > ERROR_COUNTER_TRIGGER_THRESHOLD && millis() - lastErrorCountTime > ERROR_COUNTER_DEBOUNCE_MS) {
            errorCount++;
            lastErrorCountTime = millis();
          }
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
          //idea of where it the axis is to re-initialise
          double position = stepper.get_axis_position_mm(encoderAxis);
          long positionInTicks = position * ENCODER_TICKS_PER_MM;

          //shift position from previous to current position
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

  if(invertDirection) {
    return -encoderCount.val;
  } else {
    return encoderCount.val;
  }

}

byte I2cEncoder::get_magnetic_strength() {

    //Set module to report magnetic strength
    Wire.beginTransmission((int)i2cAddress);
    Wire.write(I2C_SET_REPORT_MODE);
    Wire.write(I2C_ENC_REPORT_MODE_STRENGTH);
    Wire.endTransmission();

    //Read value
    Wire.requestFrom((int)i2cAddress,1);

    byte reading = 99;

    reading = Wire.read();

    //Set module back to normal (distance) mode
    Wire.beginTransmission((int)i2cAddress);
    Wire.write(I2C_SET_REPORT_MODE);
    Wire.write(I2C_ENC_REPORT_MODE_DISTANCE);
    Wire.endTransmission();

    return reading;
  }

bool I2cEncoder::test_axis() {
  float startPosition, endPosition;

  float startCoord[NUM_AXIS] = {0};
  float endCoord[NUM_AXIS] = {0};

  startPosition = sw_endstop_min[get_axis()] + 10;
  endPosition = sw_endstop_max[get_axis()] - 10;  

  const float homing_feedrate[] = HOMING_FEEDRATE;
  int feedrate = homing_feedrate[get_axis()] / 60;

  errorCorrect = false;

  for(int i = 0; i < NUM_AXIS; i++) {
    startCoord[i] = stepper.get_axis_position_mm((AxisEnum) i);
    endCoord[i] = stepper.get_axis_position_mm((AxisEnum) i);
  }

  startCoord[get_axis()] = startPosition;
  endCoord[get_axis()] = endPosition;

  stepper.synchronize();

  planner.buffer_line(startCoord[X_AXIS],startCoord[Y_AXIS],startCoord[Z_AXIS], stepper.get_axis_position_mm(E_AXIS), feedrate, 0);
  stepper.synchronize();

  //if the module isn't currently trusted, wait until it is (or until it should be if things are working)
  if(trusted == false) {
    long startWaitingTime = millis();
    while(!trusted && millis() - startWaitingTime < STABLE_TIME_UNTIL_TRUSTED) {
      idle();
      delay(500);
    }
  }

  if(trusted) {
    //if trusted, commence test
    planner.buffer_line(endCoord[X_AXIS],endCoord[Y_AXIS],endCoord[Z_AXIS], stepper.get_axis_position_mm(E_AXIS), feedrate, 0);
    stepper.synchronize();

    if(trusted) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

void I2cEncoder::calibrate_steps_mm(int iterations) {
  float oldStepsMm, newStepsMm, startDistance, endDistance, travelDistance, travelledDistance, total = 0;
  long startCount, stopCount;

  float startCoord[NUM_AXIS] = {0};
  float endCoord[NUM_AXIS] = {0};


  const float homing_feedrate[] = HOMING_FEEDRATE;
  int feedrate = homing_feedrate[get_axis()] / 60;

  if(get_axis() == X_AXIS || get_axis() == Y_AXIS || get_axis() == Z_AXIS) {

    errorCorrect = false;

    startDistance = 20;
    endDistance = sw_endstop_max[get_axis()] - 20;
    travelDistance = endDistance - startDistance;

    for(int i = 0; i < NUM_AXIS; i++) {
      startCoord[i] = stepper.get_axis_position_mm((AxisEnum) i);
      endCoord[i] = stepper.get_axis_position_mm((AxisEnum) i);
    }

    startCoord[get_axis()] = startDistance;
    endCoord[get_axis()] = endDistance;


    for(int i = 0; i < iterations; i++) {
      stepper.synchronize();

      planner.buffer_line(startCoord[X_AXIS],startCoord[Y_AXIS],startCoord[Z_AXIS], stepper.get_axis_position_mm(E_AXIS), feedrate, 0);
      stepper.synchronize();

      delay(250);
      startCount = get_position();

      //do_blocking_move_to(endCoord[X_AXIS],endCoord[Y_AXIS],endCoord[Z_AXIS]);

      planner.buffer_line(endCoord[X_AXIS],endCoord[Y_AXIS],endCoord[Z_AXIS], stepper.get_axis_position_mm(E_AXIS), feedrate, 0);
      stepper.synchronize();

      //Read encoder distance

      delay(250);
      stopCount = get_position();

      travelledDistance = mm_from_count(abs(stopCount - startCount));

      SERIAL_ECHO("Attempted to travel: ");
      SERIAL_ECHO(travelDistance);
      SERIAL_ECHOLN("mm.");

      SERIAL_ECHO("Actually travelled:  ");
      SERIAL_ECHO(travelledDistance);
      SERIAL_ECHOLN("mm.");

      //Calculate new axis steps per unit
      oldStepsMm = planner.axis_steps_per_mm[get_axis()];
      newStepsMm = (oldStepsMm * travelDistance) / travelledDistance;

      SERIAL_ECHO("Old steps per mm: ");
      SERIAL_ECHOLN(oldStepsMm);

      SERIAL_ECHO("New steps per mm: ");
      SERIAL_ECHOLN(newStepsMm);
      
      //Save new value
      planner.axis_steps_per_mm[get_axis()] = newStepsMm;

      if(iterations > 1) {
        total += newStepsMm;

        //swap start and end points so next loop runs from current position
        float tempCoord = startCoord[get_axis()];
        startCoord[get_axis()] = endCoord[get_axis()];
        endCoord[get_axis()] = tempCoord;
      }
    }

    if(iterations > 1) {
      total = total / (float) iterations;
      SERIAL_ECHO("Average steps per mm: ");
      SERIAL_ECHOLN(total);

      //Save new value
      //axis_steps_per_unit[get_axis()] = total;
    }

    errorCorrect = true;

    SERIAL_ECHOLN("Calculated steps per mm has been set. Please save to EEPROM (M500) if you wish to keep these values.");
  } else {
    SERIAL_ECHOLN("Automatic steps / mm calibration not supported for this axis.");
  }

}

void I2cEncoder::set_zeroed() {
  //Set module to report magnetic strength
  Wire.beginTransmission(i2cAddress);
  Wire.write(I2C_RESET_COUNT);
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

int I2cEncoder::get_error_count() {
  return errorCount;
}

void I2cEncoder::set_error_correction_enabled(bool enabled) {
  errorCorrect = enabled;
}

bool I2cEncoder::get_error_correction_enabled() {
  return errorCorrect;
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

void EncoderManager::test_axis(AxisEnum axis) {
  bool responded = false;
  for(byte i = 0; i < NUM_AXIS; i++) {
    if(encoderArray[i].get_axis() == axis) {
      encoderArray[i].test_axis();
      responded = true;
      break;
    }
  }
}

void EncoderManager::test_axis() {
  for(byte i = 0; i < NUM_AXIS; i++) {
    test_axis((AxisEnum)i);
  }
}

void EncoderManager::calibrate_steps_mm(AxisEnum axis, int iterations) {
  bool responded = false;

  for(byte i = 0; i < NUM_AXIS; i++) {
    if(encoderArray[i].get_axis() == axis) {
      encoderArray[i].calibrate_steps_mm(iterations);
      responded = true;
      break;
    }

    if (!responded) {
      SERIAL_ECHOLN("No encoder configured for given axis!");
      responded = true;
    }
  }
}

void EncoderManager::calibrate_steps_mm(int iterations) {
  bool responded = false;

  for(byte i = 0; i < NUM_AXIS; i++) {
    if(encoderArray[i].get_active()) {
      encoderArray[i].calibrate_steps_mm(iterations);
    }
  }
}

void EncoderManager::change_module_address(int oldAddress, int newAddress) {
  byte error;

  //first check 'new' address is not in use
  Wire.beginTransmission(newAddress);
  error = Wire.endTransmission();

  if(error == 0) {
    SERIAL_ECHOLN("Warning! There is already a device with that address on the I2C bus!");
  } else {

    //now check that we can find the module on the old address
    Wire.beginTransmission(oldAddress);
    error = Wire.endTransmission();

    if(error == 0) {

      SERIAL_ECHO("Module found at ");
      SERIAL_ECHO(oldAddress);
      SERIAL_ECHOLN(", changing address...");

      //change the modules address
      Wire.beginTransmission(oldAddress);
      Wire.write(I2C_SET_ADDR);
      Wire.write(newAddress);
      Wire.endTransmission();

      SERIAL_ECHOLN("Address changed, waiting for confirmation...");

      //Wait for the module to reset (can probably be improved by polling address instead of blindly waiting...)
      long startWaiting = millis();
      while(millis() - startWaiting < REBOOT_TIME) {
        idle();
        delay(500);
      }

      //look for the module at the new address
      Wire.beginTransmission(newAddress);
      error = Wire.endTransmission();

      if(error == 0) {
        SERIAL_ECHOLN("Confirmed! Address change succesful.");
      } else {
        SERIAL_ECHOLN("Failed. Please check encoder module.");
      }

    } else {
      SERIAL_ECHOLN("No module detected!");
    }
  }
}


void EncoderManager::check_module_firmware(int address) {

  //first check there is a module
  Wire.beginTransmission(address);
  int error = Wire.endTransmission();

  if(error != 0) {
    SERIAL_ECHOLN("Warning! No module detected at given address!");
  } else {

    SERIAL_ECHO("Requesting version info from module at address ");
    SERIAL_ECHOLN(address);

    Wire.beginTransmission(address);

    Wire.write(I2C_SET_REPORT_MODE);
    Wire.write(I2C_ENC_REPORT_MODE_VERSION);
    Wire.endTransmission();

    //Read value
    Wire.requestFrom((int)address,32);

    byte temp[32] = {0};
    int tempIndex = 0;

    while(Wire.available() > 0) {
      temp[tempIndex] = Wire.read();
      tempIndex++;
    }

    for(int i = 0; i < tempIndex+1; i++) {
      if((char)temp[i] > 0) {
        SERIAL_ECHO((char)temp[i]);
      }
    }

    //Set module back to normal (distance) mode
    Wire.beginTransmission((int)address);
    Wire.write(I2C_SET_REPORT_MODE);
    Wire.write(I2C_ENC_REPORT_MODE_DISTANCE);
    Wire.endTransmission();

  }
}

void EncoderManager::report_error_count(AxisEnum axis) {

  for(byte i = 0; i < NUM_AXIS; i++) {
    if(encoderArray[i].get_axis() == axis) {
      SERIAL_ECHO("Error count on ");
      SERIAL_ECHO(axis_codes[axis]);
      SERIAL_ECHO(" axis is ");
      SERIAL_ECHO(encoderArray[i].get_error_count());
      SERIAL_ECHOLN(" events.");
      break;
    }
  }
}

void EncoderManager::report_error_count() {
  for(int i = 0; i < NUM_AXIS; i++) {
    report_error_count((AxisEnum)i);
  }
}

void EncoderManager::toggle_error_correction(AxisEnum axis) {

  for(byte i = 0; i < NUM_AXIS; i++) {
    if(encoderArray[i].get_axis() == axis) {
      encoderArray[i].set_error_correction_enabled(!encoderArray[i].get_error_correction_enabled());
      SERIAL_ECHO("Error correction on ");
      SERIAL_ECHO(axis_codes[axis]);
      SERIAL_ECHO(" axis is ");
      if(encoderArray[i].get_error_correction_enabled()) {
        SERIAL_ECHOLN("enabled.");
      } else {
        SERIAL_ECHOLN("disabled.");
      }
      break;
    }
  }
}

#endif