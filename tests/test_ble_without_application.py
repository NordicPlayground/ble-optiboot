## @description
## Flash controller over BLE without pre-loading an application

## @setup
## Load the files bootloader.hex and eeprom.hex into the controller using avrdude or Atmel Studio.

## @expected_output
## "Test successful"

#########################################
import subprocess
import sys
import time

avrdude = ['avrdude', '-p', 'm328p', '-P', 'COM4', '-c', 'arduino', '-b', '115200', '-u', '-V']
verify = ['-U', 'flash:v:application.hex']

log = open('test.log', 'w')

# Write application using BLE
try:
  flash = ['ipy', 'system_tests/test_ble/memu/memu_OTA_DFU.py', 'application.hex', 'valid']
  subprocess.check_call(flash, stdout=log, stderr=log)
except subprocess.CalledProcessError:
  print "Failed to load application over BLE"
  log.close()
  sys.exit()

time.sleep(10)

# Validate flash contents
try:
  subprocess.check_call(avrdude + verify, stdout=log, stderr=log)
except subprocess.CalledProcessError:
  print "Failed to verify application loaded over BLE"
  log.close()
  sys.exit()

print "Test successful"