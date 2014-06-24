import os
import sys
import time

SEARCH_STRING = 'm_dfu_state'
class DfuStateCheck():
    dfu_state_address = ''
    def __init__(self, map_file=r'C:\w\r\NRFFOSDK_1580_BLE_OTA_DFU_round1\Nordic\nrf51\Board\nrf6310\device_firmware_updates\bootloader\arm\_build\bootloader.map'):
            fp = open(map_file, 'r')
            word_array = []
            for each_line in fp.readlines():
                word_array = each_line.split()
                if word_array != []:
                    if word_array[0] == SEARCH_STRING:
                        self.dfu_state_address = str(word_array[1])

    def check_state_transition(self, expected_state=None):
        try:
            retval = True
            if self.dfu_state_address != '' and expected_state != None:
                # nrfjprog --memrd 0x20002040 --n 1 --w 32 --snr 518000709
                command = [NRFJPROG_PATH, '--snr', SEGGER_NO , '--memrd', self.dfu_state_address, '--n', '1', '--w', '32']
                res = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                (stdout, _stderr) = res.communicate()
                data_array = stdout.split()
                for each in data_array:
                    self.logHandler.log(":%s:" % each)

                if (int(data_array[1]) != expected_state):
                    retval = False
            else:
                raise Exception ('Either dfu_state_address or expected_state is not set')

        except Exception, e1:
            self.logHandler.log('Exception check_state_transition %s' % str(e1))
            retval = False

        finally:
            return retval