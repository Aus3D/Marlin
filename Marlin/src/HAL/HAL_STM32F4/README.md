## This HAL is for STM32F4 boards using the STM32Generic Arduino Core. 

The HAL is still in development and targets the PicoPrint control board featuring an STM32F446VET6, but should be largely cross-compatible with other STM32F4 parts and, to an extent, other STM32 parts, using the STM32Generic Arduino Core.

## How do I setup PicoPrint?

On the software side, you will need:

* The STM32Generic Core with STM32F446 support ([here](https://github.com/chrissbarr/STM32GENERIC/tree/F446VE))
* The TMC26XX Stepper Library modified for STM32Generic ([here](https://github.com/Aus3D/TMC26XStepper/tree/PicoPrint))
* Marlin with the F4 HAL (you are here)

### How do I program PicoPrint?

If the board you received already has a USB bootloader present, you can program it directly over USB from the Arduino IDE, as you would any other Arduino board. 

If the board did not come with a bootloader, you can either:

* Flash the bootloader over USB using the process described below
OR
* Program the board using an STM ST-Link programmer (or equivalent tool)


### Flashing USB bootloader to empty board
To flash the USB bootloader to an empty board, you will need:
* The PicoPrint Board
* A micro-USB cable
* The bootloader firmware .dfu file ([here](https://github.com/chrissbarr/PicoPrint-Bootloader/releases))
* The DfuSeDemo application ([here](http://www.st.com/en/development-tools/stsw-stm32080.html), bottom of page - "Get Software")

Depending on how lucky you are with which drivers are automatically installed, you will likely also need the Zadig USB driver selection tool, [here](http://zadig.akeo.ie/).

### Main developers:
@Chrissbarr


### Most up to date repository for this HAL:
https://github.com/Aus3D/Marlin/tree/bugfix-2.0.x-F446-PicoPrint

PRs should be first sent to that fork, and once tested merged to Marlin bugfix-2.0.x branch.


