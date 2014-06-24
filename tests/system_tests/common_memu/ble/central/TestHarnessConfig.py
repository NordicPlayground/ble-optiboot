#!/usr/bin/python

import sys
import getopt
import os

# string constants
ROLE_DUT = "DUT"
ROLE_TST = "TST"
TARGET_EXE = "EXE"
TARGET_HEX = "HEX"

# Defines for flash programming modes
FLASH_PROG_NONE             = 0
FLASH_PROG_PYFLASH          = 1
FLASH_PROG_NRF6310_8001     = 2
FLASH_PROG_NRF6310_8200     = 3
FLASH_PROG_NRF6310_BOTH     = 4

ERROR_MESSAGE_PYFLASH       = "Error"
ERROR_MESSAGE_NRF6310CLI    = "FAILED"

class TestHarnessConfigRadio:
    def __init__(self, btle_addr, board_id, radio_version, UartComPort):
        self.btle_addr = btle_addr
        self.board_id = board_id
        self.radio_version = radio_version
        self.UartComPort = UartComPort

class TestHarnessConfig:

    def __init__(self, options):
        self.dut_ftdi = "NO65VKJR"      #FTDI Attached to test computer buildserver-vm3
        self.tst_ftdi = "NO65VKJR"      #FTDI Attached to test computer buildserver-vm3

        # self.dut_ftdi = "D0012AACFCAE"     #FTDI Attached to leas' computer
        # self.tst_ftdi = "D0012AACFCAE"     #FTDI Attached to leas' computer

        self.tst_hex = "t:\\xpbuild\\wibree\\host\\harness\\hex\\dhspi.hex"
        self.nRF8200_tests_path = os.path.abspath("..\\..\\..\\Tests\\Automated\\nRF8200\\")
        self.nRF6310_cli = os.path.abspath("C:\\Program Files (x86)\\Nordic Semiconductor\\nRFgo Studio\\nrfgocli.exe")
        self.keil_cli = os.path.abspath("c:\Keil\UV4\Uv4.exe")
        self.JlinkExe = os.path.abspath("C:\Keil\ARM\Segger\JLink.exe")
        self.nRF6310_board_ID = "0"
        self.nRF8001_target = "ISP"
        self.nRF8001_module_type = "8001"
        self.nRF8200_target = "MOUDULE"
        #self.parser_8200_path = r"C:\wibree_host\trunk\Playground\bih\testharness8200test\\"
        self.debugPanelVersion = "D09_6342"
        self.ironPythonExe = r"C:\\Program Files (x86)\\IronPython 2.7\\ipy.exe"

        def get_latest_memu_path(self, basepath=r'C:\Program Files (x86)\Nordic Semiconductor\Master Emulator'):
            dirsandfiles = os.listdir(basepath)
            dirs = []
            for element in dirsandfiles:
                if os.path.isdir(os.path.join(basepath, element)):
                    dirs.append(element)
            if len(dirs) == 0:
                raise Exception('Master Emulator directory not found.')
            dirs.sort()
            return os.path.join(basepath, dirs[-1])

        masterApiBasePath=r'C:\Program Files (x86)\Nordic Semiconductor\Master Emulator'
        self.masterApiPath = get_latest_memu_path(masterApiBasePath)
        radioCx = TestHarnessConfigRadio('D0BB7E18E54A', 0, 'Cx', None)
        radioDx = TestHarnessConfigRadio('F851B07A85DE', 2, 'Dx', 'Com4')
        radio_drgn = TestHarnessConfigRadio('D647C7689ECE', 0, 'nRF51822', 'COM39')
        #self.radio = [radioCx, radioDx]
        self.radio = [radio_drgn]
        self.netlist_file = ""
        self.role = ROLE_DUT
        self.target = TARGET_HEX
        if (options != None):
            self.update(options.split())
        self.tst_output_file = "c:\\tmp\\testharness\\tmp_host_harness_tst_output.txt"
        self.tst_log_file = "c:\\tmp\\testharness\\tmp_host_harness_tst_log.txt"

    def usage(self):
        print
        print 'TestHarnessConfig'
        print 'todo'
        print

    def update(self, argv):
        # Parse supplied options
        try:
            # Parse and check validity of all input arguments
            opts, args = getopt.getopt(argv, "n:m:p:q:x:r:t:h")
        except getopt.GetoptError:
            self.usage()
            raise(Exception("Invalid command line arguments!"))
            return False

        # Parse the valid input arguments
        for opt, arg in opts:
            if opt == '-n':
                self.dut_ftdi = arg
            elif opt == '-m':
                self.tst_ftdi = arg
            elif opt == '-p':
                self.dut_normal_uart = arg
            elif opt == '-q':
                self.dut_loop_uart = arg
            elif opt == '-x':
                self.tst_hex = arg
            elif opt == '-r':
                if (arg == ROLE_TST):
                    self.role = ROLE_TST
            elif opt == '-t':
                if (arg == TARGET_EXE):
                    self.target = TARGET_EXE
            elif opt == '-h':
                self.usage()
                return False

        return True
