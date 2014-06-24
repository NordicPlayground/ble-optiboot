## @description .

## @setup

## @expected_output


#########################################

import clr
import os
import System
import sys
curPath = os.getcwd()
sys.path.append(curPath)
clr.AddReferenceToFile("MasterEmulator.dll")
import Nordicsemi
import getopt



class bleScan:
    def __init__(self, master, argv):
        self.btle_addr = None
        self.master = master
        # Parse command line options
        # Parse and check validity of all input arguments
        opts, args = getopt.getopt(argv,  "a:h")

        if len(opts) != 0:
          # Parse the valid input arguments
          for opt, arg in opts:
              if opt == '-a':
                  self.btle_addr = arg
              elif opt == '-h':
                  print
                  print '\noptions:'
                  print '-a        : bluetooth address of the DUT'
                  print '--'
                  print

    def discoverPeerNamed(self, name) :
        peerDevice = None
        if None == self.btle_addr  :
            return None
        foundDevices = self.master.DiscoverDevices()
        if None == foundDevices or len(foundDevices) == 0:
            return None

        for device in foundDevices:
            # print "device %r"%device.DeviceInfo
            deviceName = None
            if Nordicsemi.DeviceInfoType.CompleteLocalName in device.DeviceInfo :
                deviceName = device.DeviceInfo[Nordicsemi.DeviceInfoType.CompleteLocalName]
                print "device CompleteLocalName %r address %r"%(deviceName, device.DeviceAddress.Value)
            elif Nordicsemi.DeviceInfoType.ShortenedLocalName in device.DeviceInfo:
                deviceName = device.DeviceInfo[Nordicsemi.DeviceInfoType.ShortenedLocalName]
                print "device ShortenedLocalName %r address %r"%(deviceName, device.DeviceAddress.Value)
            if None != deviceName :
                if   name == deviceName and self.btle_addr == device.DeviceAddress.Value:
                    peerDevice = device
                    break
        return peerDevice

    def discoverPeerAddress(self, time) :
        peerDevice = None
        if None == self.btle_addr  :
            return None
        foundDevices = self.master.DiscoverDevices(time)
        if None == foundDevices or len(foundDevices) == 0:
            return None

        for device in foundDevices:
            if self.btle_addr == device.DeviceAddress.Value :
                peerDevice = device
                break
        return peerDevice
