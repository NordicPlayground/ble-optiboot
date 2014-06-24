## @description .

## @setup

## @expected_output


#########################################
import os
import sys
import time
import System
import platform
import subprocess
import clr
import pickle
from dfu_code_defines import *
from hex_to_dfupacket import HexToDFUPkts
from memu_OTA_DFU_base import BleDFUTester

common_folder=os.path.join(os.path.realpath(__file__)[0:os.path.realpath(__file__).index('system_tests')+12], 'common_memu\\ble\\central')
sys.path.append(common_folder)
import mapiTestUtils
import bleChar

masterApiBasePath=r'C:\Program Files (x86)\Nordic Semiconductor\Master Emulator'
dirsandfiles = os.listdir(masterApiBasePath)
dirs = []
for element in dirsandfiles:
    if os.path.isdir(os.path.join(masterApiBasePath, element)):
        dirs.append(element)
if len(dirs) == 0:
    raise Exception('Master Emulator directory not found.')
dirs.sort()
masterApiPath = os.path.join(masterApiBasePath, dirs[-1])
sys.path.append(masterApiPath)
clr.AddReferenceToFile("MasterEmulator.dll")
import Nordicsemi

NUMBER_OF_SEND_TRIES = 5
NUMBER_OF_VALIDATE_TRIES = 8
WAIT_TIME_BETWEEN_SENDS = 5
TIMEOUT_LENGTH = 60 + 1


# Service UUID
uuidOTAService                       = Nordicsemi.BtUuid('000015301212EFDE1523785FEABCD123')

# Characteristics UUIDs
uuidOtaControlStateCharact           = Nordicsemi.BtUuid('000015311212EFDE1523785FEABCD123')
uuidOtaDfuPacketCharact              = Nordicsemi.BtUuid('000015321212EFDE1523785FEABCD123')

#Descriptor UUID
uuidClientCharactConfigDescriptor    = Nordicsemi.BtUuid(0x2902)

REQUIRED_SERVICES           = [uuidOTAService.ToString()]
REQUIRED_SOLICITED_SERVICES = []

##############################################################################################
REPORT_SIZE = 0x05
class BleDFUTests(BleDFUTester):
    def __init__(self, peerDeviceName, timeouttest):
        super(BleDFUTests, self).__init__(peerDeviceName, timeouttest)
        self.OtaDfuControlPointQ = mapiTestUtils.pipeQueue()

    def performTest(self):
        global hextosend
        global testChoice
        super(BleDFUTests, self).performBaseTest(testChoice, hextosend)



if len(sys.argv) < 3:
    raise Exception('Argument(s) missing. Usage: memu_OTA_DFU.py [abs path to hexfile] ([valid] OR [sizeTooBig] OR [invalid] OR [timeout] OR [nrfjprogreset] OR [invalidcrc]) [DUT serial no]')
else:
    hextosend=str(sys.argv[1])
    if not os.path.exists(hextosend):
        raise Exception('Hex file does not exist.')
    if ((str(sys.argv[2]) == 'valid') or
        (str(sys.argv[2]) == 'sizeTooBig') or
        (str(sys.argv[2]) == 'missingpackets') or
        (str(sys.argv[2]) == 'addedpackets') or
        (str(sys.argv[2]) == 'timeout') or
        (str(sys.argv[2]) == 'nrfjprogreset') or
        (str(sys.argv[2]) == 'invalidcrc') or
        (str(sys.argv[2]) == 'validMinimum')):
            testChoice = str(sys.argv[2])
            print "testChoice" , testChoice
    else:
        testChoice = 'valid'


if ((testChoice == 'valid') or
    (testChoice == 'sizeTooBig') or
    (testChoice == 'missingpackets') or
    (testChoice == 'addedpackets') or
    (testChoice == 'timeout') or
    (testChoice == 'nrfjprogreset') or
    (testChoice == 'invalidcrc') or
    (testChoice == 'validMinimum')):
    tester = BleDFUTests('URT', True)
else:
    tester = BleDFUTests('URT', False)
tester.run()