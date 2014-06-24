## @description
## Flash the microcontroller over UART

## @setup
## Load the file bootloader.hex into the controller using avrdude or Atmel Studio.

## @expected_output
## "Test successful"

#########################################
import subprocess
import sys
import time

avrdude = ['avrdude', '-p', 'm328p', '-P', 'COM4', '-c', 'arduino', '-b', '115200', '-u', '-V']
flash = ['-U', 'flash:w:application.hex']
verify = ['-U', 'flash:v:application.hex']

log = open('test.log', 'w')

# Write application using UART
try:
  subprocess.call(avrdude + flash, stdout=log, stderr=log)
except subprocess.CalledProcessError:
  print "Failed to load application over UART"
  log.close()
  sys.exit()

time.sleep(10)

# Validate flash contents
try:
  with open('test.log', 'w+') as outfile:
    subprocess.check_call(avrdude + verify, stdout=log, stderr=log)
except subprocess.CalledProcessError:
  print "Failed to verify application loaded over UART"
  log.close()
  sys.exit()

print "Test successful"