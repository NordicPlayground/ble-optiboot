## @description .

## @setup

## @expected_output


#########################################
import os
import sys
import math
import time
import System
import random
import platform
import subprocess
import clr
import pickle
from dfu_code_defines import *
from hex_to_dfupacket import HexToDFUPkts

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

NUMBER_OF_SEND_TRIES = 1
NUMBER_OF_VALIDATE_TRIES = 8
WAIT_TIME_BETWEEN_SENDS = 0
TIMEOUT_LENGTH = 60 + 2
MINIMUM_CONN_INTRVL = 7.5
MAXIMUM_CONN_INTRVL = 7.5
SLAVE_LATENCY       = 0
SPRVISN_TIMEOUT     = 3200

# Service UUID
uuidOTAService                       = Nordicsemi.BtUuid('000015301212EFDE1523785FEABCD123')

# Characteristics UUIDs
uuidOtaControlStateCharact           = Nordicsemi.BtUuid('000015311212EFDE1523785FEABCD123')
uuidOtaDfuPacketCharact              = Nordicsemi.BtUuid('000015321212EFDE1523785FEABCD123')
uuidOtaDfuStatusReportCharact        = Nordicsemi.BtUuid('000015331212EFDE1523785FEABCD123')

#Descriptor UUID
uuidClientCharactConfigDescriptor    = Nordicsemi.BtUuid(0x2902)

REQUIRED_SERVICES           = []
REQUIRED_SOLICITED_SERVICES = []

##############################################################################################

class BleDFUTester(mapiTestUtils.MapiTester):
    def __init__(self, peerDeviceName, timeouttest):
        if timeouttest:
            mapiTestUtils.MapiTester.__init__(self, peerDeviceName, mapiTestUtils.MapiTestResultHandler(), False, True, REQUIRED_SERVICES, REQUIRED_SOLICITED_SERVICES, False, "TestBleOtaDFU")
        else:
            mapiTestUtils.MapiTester.__init__(self, peerDeviceName, mapiTestUtils.MapiTestResultHandler(), False, False, REQUIRED_SERVICES, REQUIRED_SOLICITED_SERVICES, False, "TestBleOtaDFU")
        self.OtaDfuControlPointQ = mapiTestUtils.pipeQueue()
        self.respValue = 0
        self.expectedResponse = [] # [resp_opcode, req_opcode, resp_value, resp_param]

    def dataReceivedHandler (self, sender, e):

        if e.PipeNumber == self.rxPipeOtaDfuControlState:
            self.OtaDfuControlPointQ.put_nowait(e.PipeData)
        else:
            self.logHandler.log("Received data on unexpected pipe %r"%e.PipeNumber)

    def setupService(self):

        # Add OTA Service
        self.master.SetupAddService(uuidOTAService, Nordicsemi.PipeStore.Remote)

        # Add OTA characteristic
        self.master.SetupAddCharacteristicDefinition(uuidOtaDfuPacketCharact, 2, mapiTestUtils.createByteArray(2, 0x55))
        self.pipeOtaDfuPacket = self.master.SetupAssignPipe(Nordicsemi.PipeType.Transmit)

        self.master.SetupAddCharacteristicDefinition(uuidOtaControlStateCharact, 2, mapiTestUtils.createByteArray(2, 0x55))
        self.pipeOtaControlState = self.master.SetupAssignPipe(Nordicsemi.PipeType.TransmitWithAck)
        self.rxPipeOtaDfuControlState = self.master.SetupAssignPipe(Nordicsemi.PipeType.Receive)

        self.master.SetupAddCharacteristicDescriptor(uuidClientCharactConfigDescriptor, 2, mapiTestUtils.createByteArray(0x0, 0x0))
        self.pipeControlPointCCCD = self.master.SetupAssignPipe(Nordicsemi.PipeType.TransmitWithAck)

    def checkControlPointNotification(self, rxValue):
        #print rxValue
        resp_op_code = int(rxValue[0])

        if (resp_op_code == self.expectedResponse[0]):
            if (resp_op_code == DFUOpCodes.RESPONSE_OPCODE):
                req_opcode = int (rxValue[1])
                resp_value = int (rxValue[2])

                # Image size requested
                if (req_opcode == DFUOpCodes.REPORT_SIZE):
                    if not (resp_value  == DFUErrCodes.SUCCESS):
                        self.logHandler.log("Error code received when requesting Image size, Error Code: %x" % resp_value)
                        return False
                    else:
                        resp_param = int (rxValue[3] | (rxValue[4] << 8))
                        if ((req_opcode == self.expectedResponse[1]) and
                            (resp_value == self.expectedResponse[2]) and
                            (resp_param == self.expectedResponse[3])):
                            return True
                        else:
                            self.logHandler.log("Control Point Notification does not match the expected Response")
                            self.logHandler.log("Req opcode Expected: %x Actual: %x" % (self.expectedResponse[1], req_opcode))
                            self.logHandler.log("Resp Value Expected: %x Actual: %x" % (self.expectedResponse[2], resp_value))
                            self.logHandler.log("Resp Param Expected: %x Actual: %x" % (self.expectedResponse[3], resp_param))
                            return False

                # Error Codes
                else:
                    # Expected Error
                    if (resp_value == self.expectedResponse[2]):
                        if (req_opcode == self.expectedResponse[1]):
                            return True
                        else:
                            self.logHandler.log("Expected Error Received, but with an unexpected Req opcode: %x" % req_opcode)
                            return False
                    # Unexpected Error
                    else:
                        if (req_opcode == self.expectedResponse[1]):
                            self.logHandler.log("Unexpected Error Received, but with an expected Req opcode: %x" % req_opcode)
                        else:
                            self.logHandler.log("Unexpected Error: %x Req opcode received: %x" % (resp_value, req_opcode))
                        return False

            elif (resp_op_code == DFUOpCodes.PKT_RCPT_NOTIFY_RSP):
                resp_param = int (rxValue[1] | (rxValue[2] << 8))
                if (resp_param == self.expectedResponse[3]):
                    return True
                else:
                    self.logHandler.log("Control Point Notification does not match the expected Response")
                    self.logHandler.log("Resp Param Expected: %x Actual: %x" % (self.expectedResponse[3], resp_param))
                    return False

        else:
            self.logHandler.log("Control Point Notification does not match the expected Response")
            self.logHandler.log("Req Opcode Expected: %x Actual: %x" % (self.expectedResponse[0], resp_op_code))

        return False

    def checkIfQueueIsEmpty(self, queue, timeout):
        waiting_loop_delay=0.1
        for i in range(int(math.ceil(timeout / waiting_loop_delay))):
            if not queue.empty():
                self.logHandler.log('This was in the queue: %s' % str(queue.get_nowait()))
                self.logHandler.log('Unexpected notification received.')
                raise Exception
            time.sleep(waiting_loop_delay)
        self.logHandler.log('No notification received.')

    def DFUcommonSteps(self):
        # Setting the DFU Status Report - CCCD to 0x0001
        self.testSendData(self.pipeControlPointCCCD, System.Array[System.Byte]([0x01,0x00]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "SET CCCD to 01")

        # Initiate transition to READY state
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.START_DFU]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "TEST: INITIATE TRANSITION TO INIT STATE, START DFU")
        # Send app size
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.START_DFU, DFUErrCodes.SUCCESS, 0]
        self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](self.sizePacket), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Application size packet")
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        # Initiate transition to RECEIVE INIT PACKET state
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.RECEIVE_INIT_PACK]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "TEST: INITIATE TRANSITION TO DFU IN PROGRESS STATE, RECEIVE_INIT_PACK")

        # Send Init Packet. First check if there is no messages in the queue, send the crc init packet and then validate.
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.RECEIVE_INIT_PACK, DFUErrCodes.SUCCESS, 0]
        self.checkIfQueueIsEmpty(self.OtaDfuControlPointQ,1)
        self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](self.crcPacket), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Application size packet")
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")


    # Transmit a valid image
    def performValidTest(self):
        #Perform common steps of DFU
        self.DFUcommonSteps()

        # Jumping to RECEIVING APP DATA-STATE
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.RECEIVE_DATA_PACK]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "TEST: INITIATE TRANSITION TO RECEIVING APPLICATION DATA STATE, RECEIVE DATA")

        # Send application data packets
        for a_single_packet in self.dataPackets[:-1]:
            self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](a_single_packet), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "  Data packet transfer")
        # Check if the queue is empty before sending the last packet.
        self.checkIfQueueIsEmpty(self.OtaDfuControlPointQ, 1)
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.RECEIVE_DATA_PACK, DFUErrCodes.SUCCESS, 0]
        self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](self.dataPackets[-1]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "  Data packet transfer")
        # Validate that the complete image has been transferred
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        # Send stop packet
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.VALIDATE, DFUErrCodes.SUCCESS, 0]
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.VALIDATE]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Sending packet 'Validate'")
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        # Send start application packet
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.ACTIVATE_SYS_RESET]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Sending packet 'Start application'", True)

    # Try sending a too big app size value
    def performTooBigSizeValueTest(self):
        # Setting the DFU Status Report - CCCD to 0x0001
        self.testSendData(self.pipeControlPointCCCD, System.Array[System.Byte]([0x01,0x00]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "SET CCCD to 01")

        # Initiate transition to READY state
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.START_DFU]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "TEST: INITIATE TRANSITION TO INIT STATE, START DFU")
        # Send app size
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.START_DFU, DFUErrCodes.DATA_SIZE_EXCEEDS_LIMIT, 0]
        self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](self.sizePacket), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Application size packet")
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        # Send start application packet
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.ACTIVATE_SYS_RESET]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Sending packet 'Start application'", True)

    # Transmit an invalid image: missing packets
    def performMissingPacketsTest(self):
        #Perform common steps of DFU
        self.DFUcommonSteps()

        # Jumping to RECEIVING APP DATA-STATE
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.RECEIVE_DATA_PACK]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "TEST: INITIATE TRANSITION TO RECEIVING APPLICATION DATA STATE, RECEIVE DATA")

        # Send application data packets, but skip two data packets halfway through.
        self.logHandler.log('Packet number %s and %s will not be sent.' % (str(self.evilPacketNum), str(self.evilPacketNum+1)))
        n=0
        for a_single_packet in self.dataPackets:
            n=n+1
            if (n == self.evilPacketNum) or (n == (self.evilPacketNum+1)):
                self.logHandler.log("Skipping hex-packet %s, loosing 160 bytes." % (str(n)))
                continue
            self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](a_single_packet), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "  Data packet transfer")

        # Send stop packet
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.VALIDATE, DFUErrCodes.INVALID_STATE, 0]
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.VALIDATE]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Sending packet 'Validate'")
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        # Send start application packet
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.ACTIVATE_SYS_RESET]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Sending packet 'Start application'", True)

    # Transmit an invalid image: additional packets
    def performAdditionalPacketsTest(self):
        #Perform common steps of DFU
        self.DFUcommonSteps()

        # Jumping to RECEIVING APP DATA-STATE
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.RECEIVE_DATA_PACK]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "TEST: INITIATE TRANSITION TO RECEIVING APPLICATION DATA STATE, RECEIVE DATA")

        # Send application data packets, but send two data packets twice.
        self.logHandler.log('Additional packets will be sent after the last packet.')
        # Send application data packets
        for a_single_packet in self.dataPackets[:-1]:
            self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](a_single_packet), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "  Data packet transfer")
        # Check if the queue is empty before sending the last packet.
        self.checkIfQueueIsEmpty(self.OtaDfuControlPointQ, 1)
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.RECEIVE_DATA_PACK, DFUErrCodes.SUCCESS, 0]
        self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](self.dataPackets[-1]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "  Data packet transfer")
        # Validate that the complete image has been transferred
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        self.logHandler.log("Sending the first two hex-packets one more time.")
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.RECEIVE_DATA_PACK, DFUErrCodes.DATA_SIZE_EXCEEDS_LIMIT, 0]
        self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](self.dataPackets[1]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "  Data packet transfer")
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.RECEIVE_DATA_PACK, DFUErrCodes.OPERATION_FAILED, 0]
        self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](self.dataPackets[2]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "  Data packet transfer")
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        # Send stop packet
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.VALIDATE, DFUErrCodes.INVALID_STATE, 0]
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.VALIDATE]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Sending packet 'Validate'")
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        # Send start application packet
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.ACTIVATE_SYS_RESET]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Sending packet 'Start application'", True)

    # Transmit a valid image, but have a 30 second wait before packet number 600
    def performTimeoutTest(self):
        #Perform common steps of DFU
        self.DFUcommonSteps()

        # Jumping to RECEIVING APP DATA-STATE
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.RECEIVE_DATA_PACK]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "TEST: INITIATE TRANSITION TO RECEIVING APPLICATION DATA STATE, RECEIVE DATA")

        # Send application data packets, but wait TIMEOUT_LENGTH halfway through.
        self.logHandler.log('Timeout will occur at packet number %s.' % (str(self.evilPacketNum)))
        n=0
        for a_single_packet in self.dataPackets:
            n=n+1
            self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](a_single_packet), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "  Data packet transfer")
            if (n == self.evilPacketNum):
                self.logHandler.log("Stopping transmission for %s seconds, causing a timeout." % (str(TIMEOUT_LENGTH)))
                time.sleep(TIMEOUT_LENGTH)
                break

    # Transmit an image and reset the receiving chip while sending
    def performNrfjprogResetTest(self):
        #Perform common steps of DFU
        self.DFUcommonSteps()

        # Jumping to RECEIVING APP DATA-STATE
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.RECEIVE_DATA_PACK]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "TEST: INITIATE TRANSITION TO RECEIVING APPLICATION DATA STATE, RECEIVE DATA")

        # Send application data packets, but perform reset halfway through.
        self.logHandler.log('Reset will occur at packet number %s.' % (str(self.evilPacketNum)))
        commandString = 'nrfjprog -s ' + self.memu_dut0_serial + ' -p'
        n=0
        for a_single_packet in self.dataPackets:
            n=n+1
            self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](a_single_packet), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "  Data packet transfer")
            if (n == self.evilPacketNum):
                self.logHandler.log("Calling nrfjprog reset... ")

                try:
                    proc = subprocess.Popen(commandString, stdout=subprocess.PIPE)
                    stdoutput, _stderroutput = proc.communicate()
                except Exception, e:
                    print e
                if stdoutput:
                    for i in stdoutput.splitlines():
                        self.logHandler.log(i)

                time.sleep(1)
                break

    # Send Invalid CRC value in the Init Packet
    def performInvalidCRCTest(self):
        invalid_crcPacket = []
        invalid_crcPacket.append(self.crcPacket[0] & 0x1010)
        invalid_crcPacket.append(self.crcPacket[1] & 0x1010)

        # Setting the DFU Status Report - CCCD to 0x0001
        self.testSendData(self.pipeControlPointCCCD, System.Array[System.Byte]([0x01,0x00]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "SET CCCD to 01")

        # Initiate transition to READY state
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.START_DFU]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "TEST: INITIATE TRANSITION TO INIT STATE, START DFU")
        # Send app size
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.START_DFU, DFUErrCodes.SUCCESS, 0]
        self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](self.sizePacket), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Application size packet")
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        # Initiate transition to RECEIVE INIT PACKET state
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.RECEIVE_INIT_PACK]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "TEST: INITIATE TRANSITION TO DFU IN PROGRESS STATE, RECEIVE_INIT_PACK")

        # Send Init Packet. First check if there is no messages in the queue, send the crc init packet and then validate.
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.RECEIVE_INIT_PACK, DFUErrCodes.SUCCESS, 0]
        self.checkIfQueueIsEmpty(self.OtaDfuControlPointQ,1)
        self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](invalid_crcPacket), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Application size packet")
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        # Jumping to RECEIVING APP DATA-STATE
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.RECEIVE_DATA_PACK]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "TEST: INITIATE TRANSITION TO RECEIVING APPLICATION DATA STATE, RECEIVE DATA")

        # Send application data packets
        for a_single_packet in self.dataPackets[:-1]:
            self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](a_single_packet), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "  Data packet transfer")

        # Check if the queue is empty before sending the last packet.
        self.checkIfQueueIsEmpty(self.OtaDfuControlPointQ, 1)
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.RECEIVE_DATA_PACK, DFUErrCodes.SUCCESS, 0]
        self.testSendData(self.pipeOtaDfuPacket, System.Array[System.Byte](self.dataPackets[-1]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "  Data packet transfer")
        # Validate that the complete image has been transferred
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")

        # Send stop packet
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, DFUOpCodes.VALIDATE, DFUErrCodes.CRC_ERROR, 0]
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.VALIDATE]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Sending packet 'Validate'")
        self.validatePipeMsg(self.OtaDfuControlPointQ, self.checkControlPointNotification, NUMBER_OF_VALIDATE_TRIES, "Validating message")
        self.expectedResponse = [DFUOpCodes.RESPONSE_OPCODE, 0, DFUErrCodes.SUCCESS, 0]

        # Send start application packet
        self.testSendData(self.pipeOtaControlState, System.Array[System.Byte]([DFUOpCodes.ACTIVATE_SYS_RESET]), NUMBER_OF_SEND_TRIES, WAIT_TIME_BETWEEN_SENDS, "Sending packet 'Start application'", True)

    def performValidMinimumTest(self):

        self.logHandler.log(" Setting Connection Parameters ")

        newConnectionParams = Nordicsemi.BtConnectionParameters()

        newConnectionParams.ConnectionIntervalMs     = (MAXIMUM_CONN_INTRVL + MINIMUM_CONN_INTRVL)/2  # new connection interval will be exactly between the max and min
        newConnectionParams.SlaveLatency             = SLAVE_LATENCY
        newConnectionParams.SupervisionTimeoutMs     = SPRVISN_TIMEOUT

        # Will wait if the encryption is ongoing. This is a limitation in the master API. The Master API does not handle the connection update process well when the encryption is ongoing.

        while self.master.IsEncryptionOngoing == True:
            print "."
        self.master.UpdateConnectionParameters(newConnectionParams)
        self.performValidTest()

    def performBaseTest(self, testChoice, hextosend):
        DFUPkts = HexToDFUPkts(hextosend)
        self.sizePacket = DFUPkts.app_size_packet
        self.dataPackets = DFUPkts.data_packets
        self.crcPacket = DFUPkts.app_crc_packet
        self.evilPacketNum=int(round(len(self.dataPackets)/4))
        #divider = random.randint(133, 400) / float(100)
        #self.evilPacketNum=int(round(len(self.dataPackets)/(float(divider))))  # Let's transfer between 25% and 75% before the error
        if testChoice   == 'valid':
            self.performValidTest()
        elif testChoice == 'sizeTooBig':
            self.sizePacket = [0, 144, 1, 0] # This won't fit
            self.performTooBigSizeValueTest()
        elif testChoice == 'missingpackets':
            self.performMissingPacketsTest()
        elif testChoice == 'addedpackets':
            self.performAdditionalPacketsTest()
        elif testChoice == 'timeout':
            self.performTimeoutTest()
        elif testChoice == 'nrfjprogreset':
            self.performNrfjprogResetTest()
        elif testChoice == 'invalidcrc':
            self.performInvalidCRCTest()
        elif testChoice == 'validMinimum':
            self.performValidMinimumTest()
        else:
            pass
