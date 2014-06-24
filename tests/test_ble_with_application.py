## @description
## Flash controller over BLE after pre-loading an application

## @setup
## Load the files bootloader.hex into the controller using avrdude or Atmel Studio.

## @expected_output
## "Test successful"

#########################################
import subprocess
import sys
import time

avrdude = ['avrdude', '-p', 'm328p', '-P', 'COM4', '-c', 'arduino', '-b', '115200', '-u', '-V']
flash = ['-U', 'flash:w:application.hex']
verify = ['-U', 'flash:v:application.hex']
verify2 = ['-U', 'flash:v:application2.hex']


log = open('test.log', 'w')

# Write application using UART
try:
  subprocess.check_call(avrdude + flash, stdout=log, stderr=log)
except subprocess.CalledProcessError:
  print "Failed to pre-load application"
  log.close()
  sys.exit()

time.sleep(10)

# Validate flash contents
try:
  subprocess.check_call(avrdude + verify, stdout=log, stderr=log)
except subprocess.CalledProcessError:
  print "Failed to verify integrity of pre-loaded application"
  log.close()
  sys.exit()

time.sleep(5)

# Write application using BLE
try:
  flash = ['ipy', 'system_tests/test_ble/memu/memu_OTA_DFU.py', 'application2.hex', 'valid']
  subprocess.check_call(flash, stdout=log, stderr=log)
except subprocess.CalledProcessError:
  print "Failed to load application over BLE"
  log.close()
  sys.exit()

time.sleep(10)

# Validate flash contents
try:
  subprocess.check_call(avrdude + verify2, stdout=log, stderr=log)
except subprocess.CalledProcessError:
  print "Failed to verify application loaded over BLE"
  log.close()
  sys.exit()

print "Test successful"
