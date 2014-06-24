#!/usr/bin/python
import clr
import os
import System
import sys
import traceback
import math
import time
import datetime
import Queue
from ConfigParser import SafeConfigParser

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
import TestHarnessConfig
import bleScan


ENABLE_NOTIFICATIONS  = System.Array[System.Byte]([0x01, 0x00])
DISABLE_NOTIFICATIONS = System.Array[System.Byte]([0x00, 0x00])
ENABLE_INDICATIONS    = System.Array[System.Byte]([0x02, 0x00])
DISABLE_INDICATIONS   = DISABLE_NOTIFICATIONS

WAITING_LOOP_DELAY                              = 0.1
WAIT_FOR_DISCONNECT_DELAY                       = 50


class LogHandler(object):
    def __init__(self, fileName, logPrefix = ""):
        self.outputFile = open(fileName, 'a')
        if logPrefix != "":
            logPrefix = "{0}: ".format(logPrefix)
        self.logPrefix  = logPrefix

    def __del__(self):
        self.outputFile.close()

    def log(self, msg):
        ct = datetime.datetime.now()
        formattedMsg = "[{0:02d}:{1:02d}:{2:02d}.{3:03d}] {4}{5}\n".format(ct.hour, ct.minute, ct.second, ct.microsecond / 1000, self.logPrefix, msg)
        sys.stdout.write(formattedMsg)
        sys.stdout.flush()
        self.outputFile.write(formattedMsg)

    def Info(self, msg):
        self.log(msg)

    def Error(self, msg):
        self.log("[ERROR] {0}".format(msg))


class EventHandlerException(object):
    def __init__(self, logHandler):
        self.logHandler = logHandler
        self.isRaised = False

    def raiseException(self, msg):
        self.logHandler.log(msg)
        self.isRaised = True

    def isExceptionRaised(self):
        return self.isRaised


class pipeQueue(Queue.Queue):
    def __init__(self, maxSize = 0):
        Queue.Queue.__init__(self, maxSize)

    def clear(self):
        while not self.empty():
            self.get_nowait()


def discoverPeerDevice(ble_scan, peerDeviceName):
    for i in range(10):
        peerDevice = ble_scan.discoverPeerNamed(peerDeviceName)
        if peerDevice != None:
            return peerDevice
    else:
        raise(Exception("Peer device %s not found" % peerDeviceName))


def waitingLoopIterations(seconds):
    return int(math.ceil(seconds / WAITING_LOOP_DELAY))


def waitingLoopDelay():
    time.sleep(WAITING_LOOP_DELAY)


def pipeDataAsString(pipeData, length):
    char = ""
    ret_string = ""
    i = 0
    if len(pipeData) < length:
        print "Error: Size of pipedata is smaller. Returing"
        return None
    while i < length :   # The pipeData will convert zeros filled at the end as fillers
        char ="%s" % pipeData[i]  # This roundabout conversion was because it was not possible to directly convert a byte array to string!
        if i == 0:
            ret_string = "%c" % (int(char)) # Overwrite the null character for the first iteration.
        else:
            ret_string += "%c" % (int(char))
        i += 1
    return ret_string


def pipeDataAsHexString(pipeData, length):
    ret = ""
    i = 0
    while i < length:
        ret += "%.2x" % pipeData[i]
        i += 1
    return ret


def readAndValidateChar(logHandler, master, pipe, expectedValue, testName, isHex = False):    # TODO: Delete this function when all mapi_test scripts are using the MapiTester class
    logHandler.log("Reading {0}".format(testName))
    if isHex:
        receivedValue = pipeDataAsHexString(master.RequestData(pipe), len(expectedValue)/2)
    else:
        receivedValue = pipeDataAsString(master.RequestData(pipe), len(expectedValue))
    if expectedValue == receivedValue:
        logHandler.log("{0} read and found to be as expected".format(testName))
    else :
        raise(Exception("Failed to validate {0} (received {1}, expected {2})".format(testName, receivedValue, expectedValue)))


def createByteArray(size, byteValue = 0x55):
    a = System.Array.CreateInstance(System.Byte, size)
    for i in range(size):
        a[i] = byteValue
    return a

def byteArrayStr(byteArray):
    return "".join("%02x" % byte for byte in byteArray)


class TestResultHandler(object):
    def __init__(self):
        self.logHandler = None

    def setLogHandler(self, logHandler):
        self.logHandler = logHandler

    def handlePass(self, testName, infoStr):
        '''Will be overridden in subclass'''
        self.logHandler.log("handlePass function should be implemented in subclass!")

    def handleFail(self, testName, infoStr):
        '''Will be overridden in subclass'''
        self.logHandler.log("handleFail function should be implemented in subclass!")


class MapiTestResultHandler(TestResultHandler):
    def __init__(self):
        TestResultHandler.__init__(self)

    def handlePass(self, testName, infoStr):
        self.logHandler.log(infoStr)

    def handleFail(self, testName, infoStr):
        raise(Exception(infoStr))


class MapiTester(object):

    cfgfile = r'C:\tmp\validation_framework.cfg'
    if not os.path.exists(cfgfile):
        raise Exception("Configuration file for validation framework not found: {0}".format(cfgfile))
    parser  = SafeConfigParser()
    parser.read(cfgfile)
    for config_section in parser.sections():
        if config_section == 'master_emulator_settings':
            for option_tuple in parser.items(config_section):
                vars()[option_tuple[0]] = option_tuple[1]
            break

    def __init__(self, peerDeviceName, testResultHandler, bondingRequired, skipDisconnect, requiredAdvServices, requiredAdvSolicitedServices, isAdvAppearence, logPrefix):
        self.peerDeviceName                      = peerDeviceName
        self.testResultHandler                   = testResultHandler
        self.bondingRequired                     = bondingRequired
        self.skipDisconnect                      = skipDisconnect
        self.requiredAdvServices                 = requiredAdvServices
        self.requiredAdvSolicitedServices        = requiredAdvSolicitedServices
        self.nb_errors                           = 0
        self.harnessConfig                       = TestHarnessConfig.TestHarnessConfig("")
        self.logHandler                          = LogHandler(self.harnessConfig.tst_output_file, logPrefix)
        self.isConnected                         = False
        self.disconnectEventExpected             = self.skipDisconnect
        self.isServiceSetup                      = True
        self.isAdvAppearence                     = isAdvAppearence
        self.numberOfConnParamUpdReqRcvd         = 0
        self.numberOfConnParamUpdRejects         = 0
        self.numberOfConnParamUpdNotResponded    = 0
        self.rejectConnectionParamUpdate         = False
        self.lastDisconnectReason                = -1
        self.testResultHandler.setLogHandler(self.logHandler)

    def logMessageHandler(self, sender, e):
        self.logHandler.log("[Emulator] %s" % e.Value)

    def dataReceivedHandler(self, sender, e):
        '''May be overridden in subclass'''
        self.logHandler.log("Received unhandled data on pipe {0} ({1})!".format(e.PipeNumber, byteArrayStr(e.PipeData)))

    def connectedHandler(self, sender, e):
        '''May be overridden in subclass. When overridden, the base class (i.e. this) function must be called.'''
        self.isConnected = True
        self.logHandler.log("Connected to peer device")

    def disconnectedHandler(self, sender, e):
        '''May be overridden in subclass. When overridden, the base class (i.e. this) function must be called.'''
        self.isConnected = False
        self.lastDisconnectReason = e.Value
        if self.disconnectEventExpected:
            self.logHandler.log("Disconnected from peer device")
            self.disconnectEventExpected = False
        else:
            self.logHandler.log("Error: Unexpected disconnection event!")
            self.nb_errors += 1


    def pipeErrorHandler(self, sender, e):
        '''May be overridden in subclass. When overridden, the base class (i.e. this) function must be called.'''
        self.logHandler.log("Pipe error received on pipe {0}. ErrorCode = {1}".format(e.PipeNumber, e.ErrorCode))
        self.nb_errors += 1

    def connectionUpdateRequestHandler(self, sender, e):
        self.logHandler.log("Connection Parameter Update Request Received")
        self.numberOfConnParamUpdReqRcvd += 1
        if (self.rejectConnectionParamUpdate == True):
            self.logHandler.log("Rejecting the request number {0}... ".format(self.numberOfConnParamUpdRejects + 1))
            self.master.SendConnectionUpdateResponse(e.Identifier,  Nordicsemi.ConnectionUpdateResponse.Rejected)
            self.numberOfConnParamUpdRejects += 1
        else:
            self.master.SendConnectionUpdateResponse(e.Identifier, Nordicsemi.ConnectionUpdateResponse.Accepted)
            newConnectionParams = Nordicsemi.BtConnectionParameters()

            newConnectionParams.ConnectionIntervalMs     = (e.ConnectionIntervalMaxMs + e.ConnectionIntervalMinMs)/2  # new connection interval will be exactly between the max and min
            newConnectionParams.SlaveLatency             = e.SlaveLatency
            newConnectionParams.SupervisionTimeoutMs     = e.ConnectionSupervisionTimeoutMs

            # Will wait if the encryption is ongoing. This is a limitation in the master API. The Master API does not handle the connection update process well when the encryption is ongoing.

            while self.master.IsEncryptionOngoing == True:
                print "."
            self.master.UpdateConnectionParameters(newConnectionParams)

    def waitForDisconnect(self, waitForDisconnectDelay = WAIT_FOR_DISCONNECT_DELAY, expectedDisconnectedReason = None):
        self.disconnectEventExpected = True
        for i in range(waitingLoopIterations(waitForDisconnectDelay)):
            if self.isConnected:
                waitingLoopDelay()
                sys.stdout.write(".")
            else:
                # Disconnect received. Check disconnect reason if needed.
                if (expectedDisconnectedReason != None):
                    if (expectedDisconnectedReason != int(self.lastDisconnectReason)):
                        self.logHandler.log("Incorrect disconnect reason. Expected = {0} Received = {1}".format(expectedDisconnectedReason, self.lastDisconnectReason))
                        raise(Exception("Incorrect disconnect reason. Expected = {0} Received = {1}".format(expectedDisconnectedReason, self.lastDisconnectReason)))
                break
        else:
            raise(Exception("Peer device did not disconnect!"))

    def disconnect(self):
        self.disconnectEventExpected = True
        return self.master.Disconnect()

    def discoverPeerDevice(self, ble_scan):
        for i in range(10):
            peerDevice = ble_scan.discoverPeerNamed(self.peerDeviceName)
            if peerDevice != None:
                return peerDevice
        else:
            raise(Exception("Peer device {0} not found".format(self.peerDeviceName)))

    def findServiceInUuidList(self, uuidStr, deviceInfo, uuidListDevInfoMember):
        uuidStr = uuidStr.replace('-','')
        if uuidListDevInfoMember in deviceInfo:
            uuidListStr = deviceInfo[uuidListDevInfoMember]
            uuidListStr = uuidListStr.replace(' ', '').lower()
            uuidStrList = uuidListStr.split(',')
            if uuidStrList.count(uuidStr) > 0:
                return True
        return False

    def verifyRequiredServices(self, deviceInfo, services, devInfoMembers):
        for uuid in services:
            uuid = uuid.lower()
            print "VERIFY_REQ_SERV", uuid
            for list in devInfoMembers:
                if self.findServiceInUuidList(uuid, deviceInfo, list):
                    break
            else:
                return False
        return True

    def validateAdvertisingData(self, device):
        servicesDevInfoMembers = set([Nordicsemi.DeviceInfoType.ServicesMoreAvailableUuid16,
                                      Nordicsemi.DeviceInfoType.ServicesCompleteListUuid16,
                                      Nordicsemi.DeviceInfoType.ServicesMoreAvailableUuid32,
                                      Nordicsemi.DeviceInfoType.ServicesCompleteListUuid32,
                                      Nordicsemi.DeviceInfoType.ServicesMoreAvaiableUuid128,
                                      Nordicsemi.DeviceInfoType.ServicesCompleteListUuid128])
        if not self.verifyRequiredServices(device.DeviceInfo, self.requiredAdvServices, servicesDevInfoMembers):
            raise(Exception("Advertising data does not contain all required service UUIDs"))

        solicitedServicesDevInfoMembers = set([Nordicsemi.DeviceInfoType.SolicitedServicesUuid16,
                                               Nordicsemi.DeviceInfoType.SolicitedServicesUuid128])
        if not self.verifyRequiredServices(device.DeviceInfo, self.requiredAdvSolicitedServices, solicitedServicesDevInfoMembers):
            raise(Exception("Advertising data does not contain all required solicited service UUIDs"))
        if self.isAdvAppearence :
            if not Nordicsemi.DeviceInfoType.Appearance in device.DeviceInfo:
                raise(Exception("Advertising data does not contain Appearance field"))

    def readAndValidateChar(self, pipe, expectedValue, testName, isHex = False):
        self.logHandler.log("Reading {0}".format(testName))
        if isHex:
            receivedValue = pipeDataAsHexString(self.master.RequestData(pipe), len(expectedValue)/2)
        else:
            receivedValue = pipeDataAsString(self.master.RequestData(pipe), len(expectedValue))
        if expectedValue == receivedValue:
            self.testResultHandler.handlePass(testName, "'{0}' read and found to be as expected".format(testName))
        else:
            self.testResultHandler.handleFail(testName, "Failed to validate '{0}' (received {1}, expected {2})".format(testName, receivedValue, expectedValue))

    def requestAndValidateData(self, pipe, testFunc, testName):
        self.logHandler.log("Reading {0}".format(testName))
        receivedData = self.master.RequestData(pipe)
        if testFunc(receivedData):
            self.testResultHandler.handlePass(testName, "'{0}' read and found to be as expected".format(testName))
        else:
            self.testResultHandler.handleFail(testName, "Failed to validate '{0}' (received {1})".format(testName, byteArrayStr(receivedData)))

    def isLongReadException(self, ex):
        exStr = "{0}".format(ex)
        for acceptableExceptionText in ['REQUEST_NOT_SUPPORTED', 'ATTRIBUTE_NOT_LONG']:
            if acceptableExceptionText in exStr:
                return True
        return False

    def repeatedRequestAndValidateData(self, pipe, testFunc, repeatCount, delay, testName):
        self.logHandler.log("Executing '{0}' test".format(testName))
        for i in range(repeatCount):
            time.sleep(delay)
            msg = self.master.RequestData(pipe)
            self.logHandler.log("Received {0}".format(byteArrayStr(msg)))
            if testFunc(msg):
                self.testResultHandler.handlePass(testName, "'{0}' read and found to be as expected".format(testName))
                break
        else:
            self.testResultHandler.handleFail(testName, "Failed to validate '{0}' (received {1})".format(testName, byteArrayStr(msg)))

    def requestAndValidateLongData(self, pipe, testFunc, testName, acceptLongReadError):
        self.logHandler.log("Reading {0}".format(testName))
        try:
            receivedData = self.master.RequestLongData(pipe)
        except Exception, ex:
            if self.isLongReadException(ex) and acceptLongReadError:
                self.testResultHandler.handlePass(testName, "Error: Long read not supported by slave (still passing test).")
                return
            else:
                raise(ex)
        if testFunc(receivedData):
            self.testResultHandler.handlePass(testName, "'{0}' read and found to be as expected ({1})".format(testName, byteArrayStr(receivedData)))
        else:
            self.testResultHandler.handleFail(testName, "Failed to validate '{0}' (received {1})".format(testName, byteArrayStr(receivedData)))

    def configureNotificationAndVerify(self, writePipe, readPipe, testName):
        self.logHandler.log("Verifying notification configuration commands on '{0}'".format(testName))
        self.master.SendData(writePipe, DISABLE_NOTIFICATIONS)
        readData = self.master.RequestData(readPipe)
        if readData == DISABLE_NOTIFICATIONS:
            self.master.SendData(writePipe, ENABLE_NOTIFICATIONS)
            readData = self.master.RequestData(readPipe)
            if readData == ENABLE_NOTIFICATIONS:
                self.master.SendData(writePipe, DISABLE_NOTIFICATIONS)
                self.testResultHandler.handlePass(testName, "'{0}' notification configuration commands verified successfully".format(testName))
            else:
                self.testResultHandler.handleFail(testName, "Enabling notification on '{0}' failed".format(testName))
        else:
            self.testResultHandler.handleFail(testName, "Disabling notification on '{0}' failed".format(testName))

    def sendAndVerifyData(self, writePipe, readPipe, data, testName):
        self.logHandler.log("Test write on '{0}'".format(testName))
        self.master.SendData(writePipe, data)
        time.sleep(2)
        if (self.master.RequestData(readPipe) == data):
            self.testResultHandler.handlePass(testName, "Write on '{0}' successful".format(testName))
        else:
            self.testResultHandler.handleFail(testName, "Write on '{0}' failed".format(testName))

    def validateCccdValue(self, pipe, testName):
        self.logHandler.log("Verifying {0}".format(testName))
        readData = self.master.RequestData(pipe)
        hexStr = pipeDataAsHexString(readData, len(readData))
        if hexStr == "0000" or hexStr == "0001":
            self.testResultHandler.handlePass(testName, "'{0}' value OK".format(testName))
        else:
            self.testResultHandler.handleFail(testName, "'{0}' value verification failed".format(testName))

    def testNotifications(self, pipe, queue, testFunc, recvCount, recvPeriod, noRecvPeriod, testName):
        self.logHandler.log("Testing notifications on '{0}'".format(testName))
        queue.clear()
        self.master.SendData(pipe, ENABLE_NOTIFICATIONS)

        transmissionCount = 0
        for i in range(waitingLoopIterations(recvPeriod)):
            if not queue.empty():
                transmissionCount += 1
                resp = queue.get_nowait()
                self.logHandler.log("Received notification '{0}'".format(byteArrayStr(resp)))
                if not testFunc(resp):
                    self.testResultHandler.handleFail(testName, "Response on '{0}' not as expected (received {1})".format(testName, byteArrayStr(resp)))
                    return
                if transmissionCount >= recvCount:
                    self.logHandler.log("Received expected number of transmissions on '{0}'".format(testName))
                    break
            waitingLoopDelay()
        else:
            self.testResultHandler.handleFail(testName, "Expected number of notifications on '{0}' not received in time (received {1}, expected {2})".format(testName, transmissionCount, recvCount))
            return

        self.logHandler.log("Now will disable notification and check if any notifications are received")
        self.master.SendData(pipe, DISABLE_NOTIFICATIONS)
        time.sleep(1)
        queue.clear()

        for i in range(waitingLoopIterations(noRecvPeriod)):
            if not queue.empty():
                self.testResultHandler.handleFail(testName, "Error: Some data received when it is not expected!!!")
                return
            waitingLoopDelay()
            sys.__stdout__.write(".")
        sys.__stdout__.write("\n")
        self.testResultHandler.handlePass(testName, "Done with testing Notifications on '{0}'".format(testName))

    def validatePipeMsg(self, queue, validateFunc, timeout, testName):
        for i in range(waitingLoopIterations(timeout)):
            if not queue.empty():
                msg = queue.get_nowait()
                if validateFunc(msg):
                    self.testResultHandler.handlePass(testName, "Message/response '{0}' ok".format(byteArrayStr(msg)))
                else:
                    self.testResultHandler.handleFail(testName, "Message/response '{0}' not as expected".format(byteArrayStr(msg)))
                break
            waitingLoopDelay()
        else:
            self.testResultHandler.handleFail(testName, "Message not received in time".format(testName))

    def testPipeMsg(self, queue, testFunc, timeout, testName):
        self.logHandler.log("Executing test '{0}'".format(testName))
        self.validatePipeMsg(queue, testFunc, timeout, testName)

    def testCmdResp(self, cmdPipe, respQueue, cmd, expectedResp, respTimeout, testName):
        #Send command
        self.logHandler.log("Executing command '{0}'".format(testName))
        respQueue.clear()
        self.master.SendData(cmdPipe, cmd)

        #Handle response
        self.validatePipeMsg(respQueue, lambda x: x == expectedResp, respTimeout, testName)
        time.sleep(1) #Allow any ack to be received by the slave

    def waitForEnableNotifications(self, queue, timeout, testName):
        self.logHandler.log("Executing '{0}' test".format(testName))
        for i in range(waitingLoopIterations(timeout)):
            if not queue.empty():
                msg = queue.get_nowait()
                if msg == ENABLE_NOTIFICATIONS:
                    self.testResultHandler.handlePass(testName, "Notification was enabled")
                    break
                elif msg != DISABLE_NOTIFICATIONS:
                    self.testResultHandler.handleFail(testName, "ERROR: Unknown CCCD value '{0}' received!".format(byteArrayStr(msg)))
                    break
            waitingLoopDelay()
        else:
            self.testResultHandler.handleFail(testName, "Notification was not enabled!")

    def testDisconnect(self, testName):
        self.logHandler.log("Executing '{0}' test".format(testName))
        if self.disconnect():
            self.testResultHandler.handlePass(testName, "--- Disconnected ---")
        else:
            self.testResultHandler.handleFail(testName, "Disconnect from peer device failed")

    def testReconnect(self, testName):
        self.logHandler.log("Executing '{0}' test".format(testName))
        if self.master.Connect(self.myPeerDevice.DeviceAddress):
            self.testResultHandler.handlePass(testName, "--- Reonnected ---")
        else:
            self.testResultHandler.handleFail(testName, "Impossible to re-connect to peer device")

    def testConnect(self, testName):
        self.logHandler.log("Executing '{0}' test".format(testName))
        if self.master.Connect(self.myPeerDevice.DeviceAddress):
            self.testResultHandler.handlePass(testName, "--- Connected ---")
        else:
            self.testResultHandler.handleFail(testName, "Impossible to rconnect to peer device")
    def testBond(self, testName):
        self.logHandler.log("Executing '{0}' test".format(testName))
        if self.master.Bond():
            self.testResultHandler.handlePass(testName, "--- Bonded ---")
        else:
            self.testResultHandler.handleFail(testName, "Bonding failed!")

    def testAdvertising(self, timeout, testName):
        self.logHandler.log("Executing '{0}' test".format(testName))
        if self.ble_scan.discoverPeerAddress(timeout) != None:
            self.testResultHandler.handlePass(testName, "Device is advertising")
        else:
            self.testResultHandler.handleFail(testName, "Device not advertising")

    def testNotAdvertising(self, timeout, testName):
        self.logHandler.log("Executing '{0}' test".format(testName))
        if self.ble_scan.discoverPeerAddress(timeout) == None:
            self.testResultHandler.handlePass(testName, "No advertising packet received")
        else:
            self.testResultHandler.handleFail(testName, "Advertising packet from device found while it should be in sleep")

    def testConnectionParamsUpdate(self, pipeToOpen = None, first_conn_param_update_delay = 5, max_conn_param_upd_attempts = 3, next_conn_param_upd_delay = 5, connint = None, disconnect_on_fail=True):
        self.logHandler.log("Executing tests for connection parameters update")
        time_duration_of_rejection_test_case = first_conn_param_update_delay + next_conn_param_upd_delay * (max_conn_param_upd_attempts - 1)

        if connint:
            newConnectionParams = Nordicsemi.BtConnectionParameters()
            newConnectionParams.ConnectionIntervalMs     = connint

        if self.isConnected == True:
            self.logHandler.log("Disconnecting ")
            self.disconnect()           # To kick start the connection update process from the start of the connection
            time.sleep(1)               # Wait for the disconnection to finish

        # Negative test case - Reject all requests
        self.rejectConnectionParamUpdate = True
        if connint:
            res = self.master.Connect(self.myPeerDevice.DeviceAddress, newConnectionParams)
        else:
            res = self.master.Connect(self.myPeerDevice.DeviceAddress)
        if res:
            self.logHandler.log("--- Reconnected ---")
            if self.bondingRequired:
                start = datetime.datetime.now()
                self.logHandler.log("--- Waiting for encryption completion ---")
                while self.master.IsLinkEncrypted == False:  #TODO : This does not seem to work as expected. The boolean seems to be true all the time!!
                    print ".",
                    # Wait for the encryption to be over
                    time.sleep(.1)
                    if (datetime.datetime.now() - start) > datetime.timedelta(seconds=20):
                        self.testResultHandler.handleFail("Timeout. Link did not encrypt")
                        return

            if pipeToOpen != None:
                self.master.OpenRemotePipe(pipeToOpen)
        else:
            self.testResultHandler.handleFail("Connection Params Update Reject test", "Impossible to re-connect to peer device")
            return


        self.logHandler.log("Will wait for {0} seconds for {1} connection parameters update request from slave".format(time_duration_of_rejection_test_case , max_conn_param_upd_attempts))
        for i in range(waitingLoopIterations(time_duration_of_rejection_test_case + 3)):  # next_conn_param_upd_delay is reduced because that is the time difference between the last rejected conn param update and the time when the IUT actually disconnects. Additional 3 seconds for airtime delays (for lower values of time_duration_of_rejection_test_case the waiting delay is observed to be inadequate)
            if self.numberOfConnParamUpdRejects >= max_conn_param_upd_attempts:
                if disconnect_on_fail:
                    self.logHandler.log("Rejected {0} connection params update requests. Will check if the slave initiates disconnection".format(self.numberOfConnParamUpdRejects))
                    try:
                        self.waitForDisconnect(next_conn_param_upd_delay + 2, 0x3B)   # Additional 2 second delay for caring for air time delays
                        self.testResultHandler.handlePass("Connection Params Update Reject Test", "Peer disconnected after expected number of conn param update rejects")
                        self.logHandler.log("Will try reconnecting...")
                        self.testReconnect("Reconnect after connection params update rejects")
                    except Exception, ex:
                        self.logHandler.log("[EXCEPTION] %s" % ex)
                        self.testResultHandler.handleFail("Connection Params Update Reject Test", "Peer did not disconnect despite {0} conn param update rejects".format(self.numberOfConnParamUpdRejects))
                    break
                else:
                    self.logHandler.log("Rejected {0} connection params update requests.".format(self.numberOfConnParamUpdRejects))
                    break
            waitingLoopDelay()
        else:
            self.testResultHandler.handleFail("Connection Params Update Reject Test", "Insufficient number of conn params update requests ({0}) received to perform this test. Expected number of requests = {1} in {2} seconds".format(self.numberOfConnParamUpdRejects, max_conn_param_upd_attempts, time_duration_of_rejection_test_case))

        self.rejectConnectionParamUpdate = False


        # Positive test case - Test whether the slave is attempting re-initiate connection params update procedure
        self.numberOfConnParamUpdReqRcvd = 0  # Reset the number of connection parameter updates received.

        if self.isConnected == True:
            self.logHandler.log("Disconnecting ")
            self.disconnect()           # To kick start the connection update process from the start of the connection
            time.sleep(1)  # Wait for the disconnection to finish

        if connint:
            res = self.master.Connect(self.myPeerDevice.DeviceAddress, newConnectionParams)
        else:
            res = self.master.Connect(self.myPeerDevice.DeviceAddress)
        if res:
            self.logHandler.log("--- Reconnected ---")
            if self.bondingRequired:
                self.logHandler.log("--- Waiting for encryption completion ---")
                while self.master.IsLinkEncrypted == False:  #TODO : This does not seem to work as expected. The boolean seems to be true all the time!!
                    print ".",
                    # Wait for the encryption to be over
                    time.sleep(.1)
            if pipeToOpen != None:
                self.master.OpenRemotePipe(pipeToOpen)
        else:
            self.testResultHandler.handleFail("Connection Params Update test", "Impossible to re-connect to peer device")
            return


        time.sleep(first_conn_param_update_delay + 1) # sleep an extra second to care for air time delays
        if self.numberOfConnParamUpdReqRcvd == 0:
            self.testResultHandler.handleFail("Connection Params Update test", "Slave device has not not sent a connection param update request conn param update request in the last {1} seconds".format(first_conn_param_update_delay + 1))
        else:
            self.testResultHandler.handlePass("Connection Params Update test", "Client is sending connection param update requests as expected")


    def testLinkEncrypted(self):
        return (self.master.IsLinkEncrypted)

    def testSendData(self, pipe, msg, count, delay, testName, isFailureIgnored=False):
        self.logHandler.log("Executing '{0}' test".format(testName))
        for i in range(count):
            isSendSuccessful = self.master.SendData(pipe, msg)
            if not isSendSuccessful:
                if not isFailureIgnored:
                    self.testResultHandler.handleFail(testName, "SendData failed")
            time.sleep(delay)
        else:
            if not isSendSuccessful:
                self.logHandler.log("Warning: SendData not acknowledged/failed. ")
            else:
                self.testResultHandler.handlePass(testName, "SendData test successful")

    def setupService(self):
        '''Will be overridden in subclass'''
        self.logHandler.log("Not setting up any services!")
        self.isServiceSetup = False

    def performTest(self):
        '''Will be overridden in subclass'''
        self.logHandler.log("Not running any tests!")

    def set_local_data(self):
        '''Will be overridden in subclass'''
        self.logHandler.log("Not setting local data before run!")

    def run(self):
        try:
            self.master = Nordicsemi.MasterEmulator()

            self.ble_scan = bleScan.bleScan(self.master, ['-a', self.memu_dut0_address])
            self.master.LogMessage              += self.logMessageHandler
            self.master.DataReceived            += self.dataReceivedHandler
            self.master.Connected               += self.connectedHandler
            self.master.Disconnected            += self.disconnectedHandler
            self.master.PipeError               += self.pipeErrorHandler
            self.master.ConnectionUpdateRequest += self.connectionUpdateRequestHandler

            self.logHandler.log("- - Start of script")
            retVal = self.master.EnumerateUsb()
            search_string = self.master_dongle_serial
            if search_string == None:
                raise Exception("Variable 'master_dongle_serial' not set in 'validation_framework.cfg', aborting...")
            memu_to_use = None
            for i in retVal:
                if search_string in i:
                    memu_to_use = i
                #raise(Exception("No FTDI found!"))
            self.master.Open(memu_to_use)

            self.setupService()

            # Start Master Emulator
            self.master.Run()

            try:
                self.master.LogVerbosity = Nordicsemi.Verbosity.High
            except Exception:
                pass

            self.set_local_data()

            # Run discovery procedure
            self.myPeerDevice = self.discoverPeerDevice(self.ble_scan)
            self.logHandler.log("Found peer device")
            self.validateAdvertisingData(self.myPeerDevice)

            # Connect and bond to peer device
            if self.master.Connect(self.myPeerDevice.DeviceAddress):
                self.logHandler.log("--- Connected ---")
                try:
                    if self.thisisPOPtest:
                        self.triggerPOPtest()
                except Exception:
                    pass
            else:
                raise(Exception("Connection failed!"))

            if self.bondingRequired:
                if self.master.Bond():
                    self.logHandler.log("--- Bonded ---")
                else:
                    raise(Exception("Bonding failed!"))

            if self.isServiceSetup:
                # Service discovery
                if self.master.DiscoverPipes():
                    self.logHandler.log("--- Pipes discovered ---")
                else:
                    raise(Exception("Pipe Discovery failed!"))

            # Execute actual test
            self.performTest()

            if not self.skipDisconnect:
                # Disconnect from peer
                self.logHandler.log("Will disconnect now")
                self.disconnect()

            self.waitForDisconnect()

        except Exception, ex:
            self.logHandler.log("[EXCEPTION] %s" % ex)
            self.logHandler.log("Call stack:")
            tb = traceback.extract_tb(sys.exc_info()[2])
            for f in reversed(tb):
                self.logHandler.log("  File {0}, line {1}".format(os.path.basename(f[0]), f[1]))
            self.nb_errors += 1
            if (self.master != None) and self.isConnected:
                self.disconnect()

        if (self.master != None) and self.master.IsOpen:
            self.master.Close()

        self.logHandler.log("number of errors: %i" % self.nb_errors)
