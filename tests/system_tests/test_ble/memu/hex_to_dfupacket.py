import os
import sys
import time
import System
import platform
import subprocess
import clr
import argparse
import encodings
from intelhex import IntelHex
PKT_SIZE = 20

class HexToDFUPkts():
    def __init__(self, hexfile):
        try:
            self.app_size_packet = None
            self.app_crc_packet = 0xFFFF
            self.data_packets = []

            ih = IntelHex(hexfile)
            bin_array = ih.tobinarray()
            fsize = len(bin_array)
            self.app_size_packet = [(fsize >>  0 & 0xFF),
                                    (fsize >>  8 & 0xFF),
                                    (fsize >> 16 & 0xFF),
                                    (fsize >> 24 & 0xFF)]

            for i in range(0, len(bin_array), PKT_SIZE):
                data = (bin_array[i:i+PKT_SIZE])
                self.data_packets.append(data)
                self.crc16_compute(data)

            crc_packet = self.app_crc_packet
            self.app_crc_packet = [(crc_packet >> 0 & 0xFF),
                                   (crc_packet >> 8 & 0xFF)]

        except Exception, e1:
            print "HexToDFUPkts init Exception %s" % str(e1)
            sys.exit(0)

        finally:
            return True

    def crc16_compute(self, data_array = []):
        for i in range(len(data_array)):
            # print "======================================================================================="

            tempData = (((self.app_crc_packet >> 8) & 0xFF) | (self.app_crc_packet << 8)) & 0xFFFF
            self.app_crc_packet = tempData
            # print "app_crc_packet 1 %s" % self.app_crc_packet

            self.app_crc_packet ^= data_array[i]
            # print "app_crc_packet 2 %s" % self.app_crc_packet

            tempData = ((self.app_crc_packet & 0xFF) >> 4) & 0xFF
            self.app_crc_packet ^= tempData
            # print "app_crc_packet 3 %s" % self.app_crc_packet

            tempData = ((self.app_crc_packet << 8) << 4) & 0xFFFF
            self.app_crc_packet ^= tempData
            # print "app_crc_packet 4 %s" % self.app_crc_packet

            tempData = (((self.app_crc_packet & 0xFF) << 4) << 1) & 0xFFFF
            self.app_crc_packet ^= tempData
            # print "app_crc_packet 5 %s" % self.app_crc_packet

            # print "======================================================================================="

if __name__ == "__main__":
    print "Main Called"
    # a = HexToDFUPkts('C:\Keil\ARM\Device\Nordic\\nrf51822\Board\\nrf6310\\ble\\ble_app_bps\\arm\_build\\ble_app_bps.hex')
    # a = HexToDFUPkts('C:\\w\\r\\NRFFOSDK_1493_DFU_testing_part_2_new\\test\\system_tests\\test_ble\\memu\\crc\\ble_app_hrs.hex')
