This directory contains the Optiboot small bootloader for AVR microcontrollers, 
somewhat modified specifically for the Arduino environment, and heavily modified 
to support transfer using Bluetooth Low Energy.

Optiboot is more fully described here: http://code.google.com/p/optiboot/
and is the work of Peter Knight (aka Cathedrow), building on work of Jason P
Kyle, Spiff, and Ladyada.  Arduino-specific modification are by Bill
Westfield (aka WestfW). BLE-specific modifications are by Nordic Semiconductor
ASA.

Arduino-specific issues are tracked as part of the Arduino project
at http://code.google.com/p/arduino

------------------------------------------------------------
Support for Bluetooth Low Energy
------------------------------------------------------------

Based on Optiboot revision a70f7a426c64
(https://code.google.com/p/optiboot/source/list)

NOTE: In order for this bootloader to operate correctly, the BOOTSZ0 and
BOOTSZ1 bits must be set in order to select a 2048 word boot sector size.
In Atmel Studio, this is done by setting BOOTSZ to 2048W_3800 in the fuses pane
of the device programming window and then programming the fuses.

It is recommended to set the BOOTRST fuse, as starting the device by first
running the bootloader allows both UART and BLE firmware transfer. If the user
only requires BLE support, the fuse can be cleared, so that the device starts
directly in the application. If no application is present, the device will
fall through the application part of memory and start the bootloader, which is
at the bottom of flash memory, so that UART can be used to flash an empty
device. Please note that if the device has no application, nRF8001 setup must
be loaded into OTP memory, and configuration data must be pre-loaded in EEPROM
for BLE firmware transfer to work.

Experimental support for firmware transfer using Bluetooth Low Energy has been
added. By default, the bootloader runs the normal optiboot routine in order to
flash over UART. In order to flash using Bluetooth, the bootloader depends on
SPI pin data and other information loaded into EEPROM, in order to use the
nRF8001. If valid BLE configuration data is found in the EEPROM, Bluetooth
transfer will be enabled, and either mode can be used.

The Nordicsemi Device Firmware protocol is used to update the Device Firmware from
iOS (nRF Toolbox), Android(Master Control Panel) and Windows PC(Master Control Panel).
The DFU protocol is explained here.
https://devzone.nordicsemi.com/documentation/nrf51/5.2.0/html/a00029.html

When the bootloader is run, it will wait for activity on either UART or the
nRF8001 SPI link for a short time, after which it will load the application.
If a firmware transfer is detected on either link, the bootloader will proceed
with the transfer on this link. After transfer, the application will be
loaded.

To start the application with a clean slate, we start it using a watchdog
reset. To do this, a function runs in .init3 that determines if the
application or the bootloader should be run after reset.  The same method can
be used to jump from application to bootloader if a firmware transfer over
Bluetooth is detected by an application. A special value is set in .noinit
when the transfer is detected, and a watchdog reset is done. An example of
this, named ble_uart_project_with_dfu_template, is included with the
ble-sdk-arduino library. This also shows how to store the required information
in EEPROM.

To jump from application to bootloader, or from bootloader to application,
the following procedure is used:

1. Set boot_key (in .noinit) to BOOTLOADER_KEY
2. Perform a watchdog reset
3. In a function run from .init3, if reset was caused by watchdog, and
   boot_key == BOOTLOADER_KEY,
   execute ((void (*)(void)) BOOTLOADER_START_ADDR)();

The EEPROM data is stored in the following format:

======================================
| valid application flag  (1 byte )  |
--------------------------------------
| valid nRF8001 data flag (1 byte )  |
--------------------------------------
| aci_pins_t              (12 bytes) |
--------------------------------------
| total credit            (1 byte)   |
--------------------------------------
| transfer pipes          (3 bytes)  |
--------------------------------------
| connection timeout      (2 bytes)  |
--------------------------------------
| advertise interval      (2 bytes)  |
--------------------------------------
| crc16 value             (2 bytes)  |
======================================

Integrating device firmware update capability over BLE to your Arduino sketch:
------------------------------------------------------------------------------

Add the Nordic Device Firmware update Service to your nRF8001 configuration using 
the nRFgo Studio. (The nRFgo studio is a visual editor for Bluetooth Smart and is 
a free download from nordicsemi.com) An example to add the Nordic Device Firmware 
Update Service with your BLE configuration is shown in the 
ble_uart_project_with_dfu_template project of the ble-sdk-Arduino in github.com.

The Pipe numbers for the transfer Pipes are the Pipes mapping to the 
Nordic Device Firmware update Service. The Characteristics in the Service are 
DFU Packet (Properties = Write without response, Notify) and 
DFU Control Point (Properties = Write with automatic Acknowledgment). 
The Pipe numbers are stored in the below order in the EEPROM. 
i.e. Pipe for Write without Reponse, Pipe for Notify and the Pipe for Write.

Use the bootloader_data_store() function to store the required settings to the EEPROM.
The functions stores the settings in the bottom of the EEPROM.

Use the bootloader_jump() function when the byte 0x01 is received on the 
DFU Control Point Characteristic.
This signals the start of the DFU.

Features:
---------
The firmware update over BLE is supported when the nRF8001 setup is placed in 
RAM(services.h) and in NVRAM(services_lock.h).

How to use:
----------
1. The bootloader_setup.ino file in the ble-sdk-arduino project
contains all the necessary integration so a user that requires a sketch to support
DFU over BLE needs to just add the file to their sketch folder.

2. Code added in case DEVICE_STARTED_STANDBY:

   bootloader_data_store(&aci_state, 180, 0x0050);
   
   
3. Code added in case ACI_EVT_DATA_RECEIVED: of the Arduino sketch

       case PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_CONTROL_POINT_RX_ACK_AUTO:
            if (1 == aci_evt->params.data_received.rx_data.aci_data[0] &&
                lib_aci_is_pipe_available(&aci_state, 
                                          PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_CONTROL_POINT_TX))
            {
              bootloader_jump_required = true;
            }
            break;
      
4. Code added in the loop() function in the Arduino sketch

  if (bootloader_jump_required)
  {
    bootloader_jump_required = false;
    bootloader_jump();
  }
            
Done. Your Arduino sketch is now BLE firmware update (DFU) enabled.


Limitations
-----------
This releae does not support bootloading over an encrypted link, support for this will be added
in the next release.


------------------------------------------------------------
Building optiboot for Arduino.

Production builds of optiboot for Arduino are done on a Mac in "unix mode"
using CrossPack-AVR-20100115.  CrossPack tracks WINAVR (for windows), which
is just a package of avr-gcc and related utilities, so similar builds should
work on Windows or Linux systems.

One of the Arduino-specific changes is modifications to the makefile to
allow building optiboot using only the tools installed as part of the
Arduino environment, or the Arduino source development tree.  All three
build procedures should yield identical binaries (.hex files) (although
this may change if compiler versions drift apart between CrossPack and
the Arduino IDE.)


Building Optiboot in the Arduino IDE Install.

Work in the .../hardware/arduino/bootloaders/optiboot/ and use the
"omake <targets>" command, which just generates a command that uses
the arduino-included "make" utility with a command like:
    make OS=windows ENV=arduino <targets>
or  make OS=macosx ENV=arduino <targets>
On windows, this assumes you're using the windows command shell.  If
you're using a cygwin or mingw shell, or have one of those in your
path, the build will probably break due to slash vs backslash issues.
On a Mac, if you have the developer tools installed, you can use the
Apple-supplied version of make.
The makefile uses relative paths ("../../../tools/" and such) to find
the programs it needs, so you need to work in the existing optiboot
directory (or something created at the same "level") for it to work.


Building Optiboot in the Arduino Source Development Install.

In this case, there is no special shell script, and you're assumed to
have "make" installed somewhere in your path.
Build the Arduino source ("ant build") to unpack the tools into the
expected directory.
Work in Arduino/hardware/arduino/bootloaders/optiboot and use
    make OS=windows ENV=arduinodev <targets>
or  make OS=macosx ENV=arduinodev <targets>


Programming Chips Using the _isp Targets

The CPU targets have corresponding ISP targets that will actuall
program the bootloader into a chip. "atmega328_isp" for the atmega328,
for example.  These will set the fuses and lock bits as appropriate as
well as uploading the bootloader code.

ISP Targets in Version 5.0 and later:

The isp targets are now built using a separate "Makefile.isp" makefile,
which should make modification easier and more obvious.  This also fixes
the atmega8_isp target problem mentioned below.  The default
configuration assumes an ArduinoISP setup, but you will probably need to
update at least the serial port, since those are different for each
Arduino board and/or system/


ISP Targets in Version 4.6 and earlier:

The older makefiles default to using a USB programmer, but you can use a
serial programmer like ArduinoISP by changing the appropriate variables
when you invoke make:

   make ISPTOOL=stk500v1 ISPPORT=/dev/tty.usbserial-A20e1eAN  \
        ISPSPEED=-b19200 atmega328_isp

The "atmega8_isp" target does not currently work, because the mega8
doesn't have the "extended" fuse that the generic ISP target wants to
pass on to avrdude.  You'll need to run avrdude manually.


Standard Targets

I've reduced the pre-built and source-version-controlled targets
(.hex and .lst files included in the git repository) to just the
three basic 16MHz targets: atmega8, atmega16, atmega328.
