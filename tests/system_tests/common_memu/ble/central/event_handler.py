import clr
import os
import System
import sys
import time

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

class EventHandler(object):

    def __init__(self, master):
        master.LogMessage              += self.logmessage
        master.DataReceived            += self.data_received
        master.Connected               += self.connected
        master.Disconnected            += self.disconnected
        master.PipeError               += self.pipe_error
        master.ConnectionUpdateRequest += self.connection_update_request
        self.master = master
        self.data_recv = []

    def connected(self, sender, e):
        print "connected called: {0}".format(e)


    def connection_update_request(self, sender, e):
        print "connection_update_request called: {0}".format(e)


    def data_received (self, sender, e):
        #print "data_received called: {0}".format(e.PipeData)
        data = []
        for i in e.PipeData:
            data.append(int(i))
        self.data_recv.append(data)

    def data_requested(self, sender, e):
        print "data_requested called: {0} {1}".format(sender, e)


    def device_discovered(self, sender, e):
        print "device_discovered called: {0} {1}".format(sender, e)


    def disconnected(self, sender, e):
        print "disconnected called: {0} {1}".format(sender, e.Value)


    def display_pass_key(self, sender, e):
        print "display_pass_key called: {0} {1}".format(sender, e)


    def logmessage(self, sender, e):
        print "logmessage called: {0}".format(e.Value)


    def oob_key_request(self, sender, e):
        print "oob_key_request called: {0} {1}".format(sender, e)


    def passkey_request(self, sender, e):
        print "passkey_request called: {0} {1}".format(sender, e)


    def pipe_error(self, sender, e):
        print "pipe_error called: {0} {1}".format(sender, e)


    def security_request(self, sender, e):
        print "security_request called: {0} {1}".format(sender, e)

class Profile(object):
    generic_access = {'service': 0x1800,
                      'device_name': 0x2A00,
                      'appearance': 0x2A01,
                      'slave_preferred_connection_parameters': 0x2A04,
                      }

    heart_rate      = {'service': 0x180D,
                      'measurement': 0x2A37,
                      'location': 0x2A38,
                      }

    battery         = {'service': 0x180F,
                      'measurement': 0x2A19,
                      }

    device_information = {'service': 0x180A,
                          'manufacturer_name': 0x2A29,
                         }
    def __init__(self, services):
        self.profile.services = []
        if profile == 'hrm':
            self.profile.services.append


class Service(object):
    def __init__(self, master):
        self.master = master

    def setup_hrs(self):
        pass

    def add_service(self, name):
        if name == 'hrs':
            master.SetupAddService(service_heart_rate, Nordicsemi.PipeStore.Remote)

    def get_service_name(service_name):
        return Nordicsemi.BtUuid(service_name['service'])