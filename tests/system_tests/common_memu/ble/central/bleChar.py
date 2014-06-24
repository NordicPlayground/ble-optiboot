#!/usr/bin/python
import datetime
import math


class sfloat(object):
    def __init__(self, exp, mant):
        self.exponent = exp
        self.mantissa = mant

    def encode(self, buffer, idx):
        buffer[idx] = self.mantissa & 0x00ff
        buffer[idx + 1] = (self.exponent << 4) | ((self.mantissa & 0x0f00) >> 8)

    @classmethod
    def decode(cls, buffer, idx):
        exp = (buffer[idx + 1] >> 4) & 0x0f
        mant = ((buffer[idx + 1] & 0x0f) << 8) | buffer[idx]
        return cls(exp, mant)

    @classmethod
    def default(cls):
        return cls(0, 0)

    def __eq__(self, other):
        return (self.exponent == other.exponent) and (self.mantissa == other.mantissa)

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
        return "%s(exponent=0x%02X, mantissa=0x%04X)" % (self.__class__.__name__, self.exponent, self.mantissa)

    def __str__(self):
        return self.__repr__()

    def assertValue(self, expected, name):
        if self != expected:
            raise ValueError("Unexpected value for {0} (was {1}, expected {2})".format(name, self, expected))


class bleFloat(object):
    def __init__(self, exp, mant):
        self.exponent = exp
        self.mantissa = mant

    @classmethod
    def decode(cls, buffer, idx):
        exp = buffer[idx + 3]
        mant = (buffer[idx + 2] << 16) | (buffer[idx + 1] << 8) | buffer[idx]
        return cls(exp, mant)

    @classmethod
    def default(cls):
        return cls(0, 0)

    def __eq__(self, other):
        return (self.exponent == other.exponent) and (self.mantissa == other.mantissa)

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
        return "%s(exponent=0x%02X, mantissa=0x%06X)" % (self.__class__.__name__, self.exponent, self.mantissa)

    def __str__(self):
        return self.__repr__()

    def assertValue(self, expected, name):
        if self != expected:
            raise ValueError("Unexpected value for {0} (was {1}, expected {2})".format(name, self, expected))

    def value(self):
        exp = self.exponent
        if exp > 127:
            exp = -(256 - exp)
        return self.mantissa * math.pow(10, exp)


class DateTime(datetime.datetime):
    MIN_YEAR = 1582

    def __init__(self, year, month, day, hour, minute, second):
        if year < self.MIN_YEAR:
            raise ValueError("Timestamp year out of range (%r)" % year)
        datetime.datetime.__init__(year, month, day, hour, minute, second)

    @classmethod
    def decode(cls, buffer, idx):
        year   = (buffer[idx + 1] << 8) + buffer[idx]
        month  = buffer[idx + 2]
        day    = buffer[idx + 3]
        hour   = buffer[idx + 4]
        minute = buffer[idx + 5]
        second = buffer[idx + 6]

        return cls(year, month, day, hour, minute, second)

    @classmethod
    def default(cls):
        return cls(cls.MIN_YEAR, 1, 1, 0, 0, 0)

    def assertValue(self, expected, name):
        if self != expected:
            raise ValueError("Unexpected value for {0} (was {1}, expected {2})".format(name, str(self), str(expected)))


class CscMeasurement(object):
    FLAG_WHEEL_REV_DATA = 0x01
    FLAG_CRANK_REV_DATA = 0x02
    FLAG_UNUSED          = ~(FLAG_WHEEL_REV_DATA | FLAG_CRANK_REV_DATA)

    def __init__(self, flags, cumulWheelRev, wheelEvtTime, cumulCrankRev, crankEvtTime):
        if (flags & self.FLAG_UNUSED) != 0:
            raise ValueError("RscMeasurement.flags RFU bit(s) set ({0})".format(flags))
        self.flags          = flags
        self.cumulWheelRev  = cumulWheelRev
        self.wheelEvtTime   = wheelEvtTime
        self.cumulCrankRev  = cumulCrankRev
        self.crankEvtTime   = crankEvtTime

    @classmethod
    def decode(cls, buffer, idx):
        flags = buffer[idx]
        offset = 1
        if (flags & cls.FLAG_WHEEL_REV_DATA) != 0:
            cumulWheelRev = (buffer[idx + offset + 3] << 24) + (buffer[idx + offset + 2] << 16) + (buffer[idx + offset + 1] << 8) + buffer[idx + offset]
            offset += 4
            wheelEvtTime  = (buffer[idx + offset + 1] << 8) + buffer[idx + offset]
            offset += 2
        else:
            cumulWheelRev = 0
            wheelEvtTime  = 0

        if (flags & cls.FLAG_CRANK_REV_DATA) != 0:
            cumulCrankRev = (buffer[idx + offset + 1] << 8) + buffer[idx + offset]
            offset += 2
            crankEvtTime  = (buffer[idx + offset + 1] << 8) + buffer[idx + offset]
            offset += 2
        else:
            cumulCrankRev = 0
            crankEvtTime  = 0
        return cls(flags, cumulWheelRev, wheelEvtTime, cumulCrankRev, crankEvtTime)

    def __eq__(self, other):
        if (self.flags != other.flags) :
            return False
        if ((self.flags & self.FLAG_WHEEL_REV_DATA) != 0) and ((self.cumulWheelRev != other.cumulWheelRev) or (self.wheelEvtTime != other.wheelEvtTime) ):
            return False
        if ((self.flags & self.FLAG_CRANK_REV_DATA) != 0) and ((self.cumulCrankRev != other.cumulCrankRev) or (self.crankEvtTime != other.crankEvtTime) ):
            return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
        if (self.flags & self.FLAG_WHEEL_REV_DATA) != 0:
            cumulWheelRevStr = "{0}".format(self.cumulWheelRev)
            wheelEvtTimeStr  = "{0}".format(self.wheelEvtTime)
        else:
            cumulWheelRevStr = ""
            wheelEvtTimeStr  = ""

        if (self.flags & self.FLAG_CRANK_REV_DATA) != 0:
            cumulCrankRevStr = "{0}".format(self.cumulCrankRev)
            crankEvtTimeStr  = "{0}".format(self.crankEvtTime)
        else:
            cumulCrankRevStr = ""
            crankEvtTimeStr  = ""
        return "{0}(flags=0x{1:02X}, cumulWheelRev={2}, wheelEvtTime={3}, cumulCrankRev={4}, crankEvtTime={5})".format(
                self.__class__.__name__, self.flags, cumulWheelRevStr, wheelEvtTimeStr,
                cumulCrankRevStr, crankEvtTimeStr)

    def __str__(self):
        return self.__repr__()

    def __assertFlagValue(self, expected, flag, flagName):
        if (self.flags & flag) != (expected.flags & flag):
            raise ValueError("Unexpected value for {0} (was {1}, expected {2})".format(flagName, (self.flags & flag) != 0, (expected.flags & flag) != 0))

    def assertValue(self, expected, name):
        self.__assertFlagValue(expected, self.FLAG_WHEEL_REV_DATA, name + ".flags.FLAG_WHEEL_REV_DATA")
        self.__assertFlagValue(expected, self.FLAG_CRANK_REV_DATA, name + ".flags.FLAG_CRANK_REV_DATA")
        if (self.flags & self.FLAG_WHEEL_REV_DATA) != 0:
            if self.cumulWheelRev != expected.cumulWheelRev:
                raise ValueError("Unexpected value for {0}.cumulWheelRev (was {1}, expected {2})".format(name, self.cumulWheelRev, expected.cumulWheelRev))
            if self.wheelEvtTime != expected.wheelEvtTime:
                raise ValueError("Unexpected value for {0}.wheelEvtTime (was {1}, expected {2})".format(name, self.wheelEvtTime, expected.wheelEvtTime))
        if (self.flags & self.FLAG_CRANK_REV_DATA) != 0:
            if self.cumulCrankRev != expected.cumulCrankRev:
                raise ValueError("Unexpected value for {0}.cumulCrankRev (was {1}, expected {2})".format(name, self.cumulCrankRev, expected.cumulCrankRev))
            if self.crankEvtTime != expected.crankEvtTime:
                raise ValueError("Unexpected value for {0}.crankEvtTime (was {1}, expected {2})".format(name, self.crankEvtTime, expected.crankEvtTime))

    def isWheelRevDataIncluded(self):
        return (self.flags & self.FLAG_WHEEL_REV_DATA) != 0

    def isCrankRevDataIncluded(self):
        return (self.flags & self.FLAG_CRANK_REV_DATA) != 0

class RscMeasurement(object):
    FLAG_INST_STRIDE_LEN = 0x01
    FLAG_TOTAL_DIST      = 0x02
    FLAG_RUNNING         = 0x04
    FLAG_UNUSED          = ~(FLAG_INST_STRIDE_LEN | FLAG_TOTAL_DIST | FLAG_RUNNING)

    def __init__(self, flags, instSpeed, instCadence, instStrideLen, totalDist):
        if (flags & self.FLAG_UNUSED) != 0:
            raise ValueError("RscMeasurement.flags RFU bit(s) set ({0})".format(flags))
        self.flags         = flags
        self.instSpeed     = instSpeed
        self.instCadence   = instCadence
        self.instStrideLen = instStrideLen
        self.totalDist     = totalDist

    @classmethod
    def decode(cls, buffer, idx):
        flags = buffer[idx]
        instSpeed = (buffer[idx + 2] << 8) + buffer[idx + 1]
        instCadence = buffer[idx + 3]
        offset = 4
        if (flags & cls.FLAG_INST_STRIDE_LEN) != 0:
            instStrideLen = (buffer[idx + offset + 1] << 8) + buffer[idx + offset]
            offset += 2
        else:
            instStrideLen = 0
        if (flags & cls.FLAG_TOTAL_DIST) != 0:
            totalDist = (buffer[idx + offset + 3] << 24) + (buffer[idx + offset + 2] << 16) + (buffer[idx + offset + 1] << 8) + buffer[idx + offset]
        else:
            totalDist = 0
        return cls(flags, instSpeed, instCadence, instStrideLen, totalDist)

    def __eq__(self, other):
        if (self.flags != other.flags) or (self.instSpeed != other.instSpeed) or (self.instCadence != other.instCadence):
            return False
        if ((self.flags & self.FLAG_INST_STRIDE_LEN) != 0) and (self.instStrideLen != other.instStrideLen):
            return False
        if ((self.flags & self.FLAG_TOTAL_DIST) != 0) and (self.totalDist != other.totalDist):
            return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
        if (self.flags & self.FLAG_INST_STRIDE_LEN) != 0:
            instStrideLenStr = "{0}".format(self.instStrideLen)
        else:
            instStrideLenStr = ""
        if (self.flags & self.FLAG_TOTAL_DIST) != 0:
            totalDistStr = "{0}".format(self.totalDist)
        else:
            totalDistStr = ""
        return "{0}(flags=0x{1:02X}, instSpeed={2}, instCadence={3}, instStrideLen={4}, totalDist={5})".format(
                self.__class__.__name__, self.flags, self.instSpeed,
                self.instCadence, instStrideLenStr, totalDistStr)

    def __str__(self):
        return self.__repr__()

    def __assertFlagValue(self, expected, flag, flagName):
        if (self.flags & flag) != (expected.flags & flag):
            raise ValueError("Unexpected value for {0} (was {1}, expected {2})".format(flagName, (self.flags & flag) != 0, (expected.flags & flag) != 0))

    def assertValue(self, expected, name):
        self.__assertFlagValue(expected, self.FLAG_INST_STRIDE_LEN, name + ".flags.INST_STRIDE_LEN")
        self.__assertFlagValue(expected, self.FLAG_TOTAL_DIST, name + ".flags.TOTAL_DIST")
        self.__assertFlagValue(expected, self.FLAG_RUNNING, name + ".flags.RUNNING")
        if self.instSpeed != expected.instSpeed:
            raise ValueError("Unexpected value for {0}.instSpeed (was {1}, expected {2})".format(name, self.instSpeed, expected.instSpeed))
        if self.instCadence != expected.instCadence:
            raise ValueError("Unexpected value for {0}.instCadence (was {1}, expected {2})".format(name, self.instCadence, expected.instCadence))
        if (self.flags & self.FLAG_INST_STRIDE_LEN) != 0:
            if self.instStrideLen != expected.instStrideLen:
                raise ValueError("Unexpected value for {0}.instStrideLen (was {1}, expected {2})".format(name, self.instStrideLen, expected.instStrideLen))
        if (self.flags & self.FLAG_TOTAL_DIST) != 0:
            if self.totalDist != expected.totalDist:
                raise ValueError("Unexpected value for {0}.totalDist (was {1}, expected {2})".format(name, self.totalDist, expected.totalDist))

    def isInstStrideLenIncluded(self):
        return (self.flags & self.FLAG_INST_STRIDE_LEN) != 0

    def isTotalDistIncluded(self):
        return (self.flags & self.FLAG_TOTAL_DIST) != 0

    def isRunning(self):
        return (self.flags & self.FLAG_RUNNING) != 0


class BloodPressureMeasurement(object):
    FLAG_UNIT_KPA    = 0x01
    FLAG_TIME_STAMP  = 0x02
    FLAG_PULSE_RATE  = 0x04
    FLAG_USER_ID     = 0x08
    FLAG_MEAS_STATUS = 0x10
    FLAG_UNUSED      = ~(FLAG_UNIT_KPA | FLAG_TIME_STAMP | FLAG_PULSE_RATE | FLAG_USER_ID | FLAG_MEAS_STATUS)

    def __init__(self, flags, systolic, diastolic, meanArterial, timeStamp, pulseRate, userId, measStatus):
        if (flags & self.FLAG_UNUSED) != 0:
            raise ValueError("BloodPressureMeasurement.flags RFU bit(s) set ({0})".format(flags))
        if (flags & self.FLAG_MEAS_STATUS) != 0:
            if (measStatus & 0xffc0) != 0:
                raise ValueError("RFU bits are set in BloodPressureMeasurement.MeasurementStatus (%r)" % measStatus)
            pulseRateRangeDetectionFlags = (measStatus >> 3) & 0x0003
            if pulseRateRangeDetectionFlags == 3:
                raise ValueError("RFU value used in BloodPressureMeasurement.MeasurementStatus.PulseRateRangeDetectionFlags (%r)" % measStatus)
        self.flags        = flags
        self.systolic     = systolic
        self.diastolic    = diastolic
        self.meanArterial = meanArterial
        self.timeStamp    = timeStamp
        self.pulseRate    = pulseRate
        self.userId       = userId
        self.measStatus   = measStatus

    @classmethod
    def decode(cls, buffer, idx):
        # Extract flags
        flags = buffer[idx]

        # Validate data length
        expectedLength = 7
        if (flags & cls.FLAG_TIME_STAMP) != 0:
            expectedLength += 7
        if (flags & cls.FLAG_PULSE_RATE) != 0:
            expectedLength += 2
        if (flags & cls.FLAG_USER_ID) != 0:
            expectedLength += 1
        if (flags & cls.FLAG_MEAS_STATUS) != 0:
            expectedLength += 2
        maxLen = len(buffer) - idx
        if maxLen < expectedLength:
            raise ValueError("Invalid data length (flags = {0}, expected length = {0}, actual Length = {1})".format(flags, expectedLength, maxLen))

        # Extract mandatory fields
        systolic     = sfloat.decode(buffer, idx + 1)
        diastolic    = sfloat.decode(buffer, idx + 3)
        meanArterial = sfloat.decode(buffer, idx + 5)

        # Extract conditional fields
        offset = 7
        if (flags & cls.FLAG_TIME_STAMP) != 0:
            timeStamp = DateTime.decode(buffer, idx + offset)
            offset += 7
        else:
            timeStamp = DateTime.default()

        if (flags & cls.FLAG_PULSE_RATE) != 0:
            pulseRate = sfloat.decode(buffer, idx + offset)
            offset += 2
        else:
            pulseRate = sfloat.default()

        if (flags & cls.FLAG_USER_ID) != 0:
            userId = buffer[idx + offset]
            offset += 1
        else:
            userId = 255

        if (flags & cls.FLAG_MEAS_STATUS) != 0:
            measStatus = (buffer[idx + offset + 1] << 8) + buffer[idx + offset]
        else:
            measStatus = 0xFFFF

        return cls(flags, systolic, diastolic, meanArterial, timeStamp, pulseRate, userId, measStatus)

    def __eq__(self, other):
        if (self.flags != other.flags) or (self.systolic != other.systolic) or (self.diastolic != other.diastolic) or (self.meanArterial != other.meanArterial):
            return False
        if ((self.flags & self.FLAG_TIME_STAMP) != 0) and (self.timeStamp != other.timeStamp):
            return False
        if ((self.flags & self.FLAG_PULSE_RATE) != 0) and (self.pulseRate != other.pulseRate):
            return False
        if ((self.flags & self.FLAG_USER_ID) != 0) and (self.userId != other.userId):
            return False
        if ((self.flags & self.FLAG_MEAS_STATUS) != 0) and (self.measStatus != other.measStatus):
            return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
        if (self.flags & self.FLAG_TIME_STAMP) != 0:
            timeStampStr = "{0}".format(str(self.timeStamp))
        else:
            timeStampStr = ""
        if (self.flags & self.FLAG_PULSE_RATE) != 0:
            pulseRateStr = "{0}".format(self.pulseRate)
        else:
            pulseRateStr = ""
        if (self.flags & self.FLAG_USER_ID) != 0:
            userIdStr = "{0}".format(self.userId)
        else:
            userIdStr = ""
        if (self.flags & self.FLAG_MEAS_STATUS) != 0:
            measStatusStr = "0x{0:04X}".format(self.measStatus)
        else:
            measStatusStr = ""
        return "{0}(flags=0x{1:02X}, systolic={2}, diastolic={3}, meanArterial={4}, timeStamp={5}, pulseRate={6}, userId={7}, measStatus={8})".format(
                self.__class__.__name__, self.flags, self.systolic, self.diastolic,
                self.meanArterial, timeStampStr, pulseRateStr, userIdStr, measStatusStr)

    def __str__(self):
        return self.__repr__()

    def __assertFlagValue(self, expected, flag, flagName):
        if (self.flags & flag) != (expected.flags & flag):
            raise ValueError("Unexpected value for {0} (was {1}, expected {2})".format(flagName, (self.flags & flag) != 0, (expected.flags & flag) != 0))

    def assertValue(self, expected, name):
        self.__assertFlagValue(expected, self.FLAG_UNIT_KPA, name + ".flags.UNIT_KPA")
        self.__assertFlagValue(expected, self.FLAG_TIME_STAMP, name + ".flags.TIME_STAMP")
        self.__assertFlagValue(expected, self.FLAG_PULSE_RATE, name + ".flags.PULSE_RATE")
        self.__assertFlagValue(expected, self.FLAG_USER_ID, name + ".flags.USER_ID")
        self.__assertFlagValue(expected, self.FLAG_MEAS_STATUS, name + ".flags.MEAS_STATUS")
        self.systolic.assertValue(expected.systolic, name + ".systolic")
        self.diastolic.assertValue(expected.diastolic, name + ".diastolic")
        self.meanArterial.assertValue(expected.meanArterial, name + ".meanArterial")
        if (self.flags & self.FLAG_TIME_STAMP) != 0:
            self.timeStamp.assertValue(expected.timeStamp, name + ".timeStamp")
        if (self.flags & self.FLAG_PULSE_RATE) != 0:
            self.pulseRate.assertValue(expected.pulseRate, name + ".pulseRate")
        if (self.flags & self.FLAG_USER_ID) != 0:
            if self.userId != expected.userId:
                raise ValueError("Unexpected value for {0}.userId (was {1}, expected {2})".format(name, self.userId, expected.userId))
        if (self.flags & self.FLAG_MEAS_STATUS) != 0:
            if self.measStatus != expected.measStatus:
                raise ValueError("Unexpected value for {0}.measStatus (was {1}, expected {2})".format(name, self.measStatus, expected.measStatus))

    def isUnitKpa(self):
        return (self.flags & self.FLAG_UNIT_KPA) != 0

    def isTimeStampIncluded(self):
        return (self.flags & self.FLAG_TIME_STAMP) != 0

    def isPulseRateIncluded(self):
        return (self.flags & self.FLAG_PULSE_RATE) != 0

    def isUserIdIncluded(self):
        return (self.flags & self.FLAG_USER_ID) != 0

    def isMeasurementStatusIncluded(self):
        return (self.flags & self.FLAG_MEAS_STATUS) != 0


class TemperatureMeasurement(object):
    FLAG_UNIT_FAHRENHEIT = 0x01
    FLAG_TIME_STAMP      = 0x02
    FLAG_TEMP_TYPE       = 0x04
    FLAG_UNUSED          = ~(FLAG_UNIT_FAHRENHEIT | FLAG_TIME_STAMP | FLAG_TEMP_TYPE)

    def __init__(self, flags, tempMeasurement, timeStamp, tempType):
        if (flags & self.FLAG_UNUSED) != 0:
            raise ValueError("TemperatureMeasurement.flags RFU bit(s) set ({0})".format(flags))
        self.flags           = flags
        self.tempMeasurement = tempMeasurement
        self.timeStamp       = timeStamp
        self.tempType        = tempType

    @classmethod
    def decode(cls, buffer, idx):
        # Extract flags
        flags = buffer[idx]

        # Validate data length
        expectedLength = 5
        if (flags & cls.FLAG_TIME_STAMP) != 0:
            expectedLength += 7
        if (flags & cls.FLAG_TEMP_TYPE) != 0:
            expectedLength += 1
        maxLen = len(buffer) - idx
        if maxLen < expectedLength:
            raise ValueError("Invalid data length (flags = {0}, expected length = {0}, actual Length = {1})".format(flags, expectedLength, maxLen))

        # Extract mandatory fields
        tempMeasurement = bleFloat.decode(buffer, idx + 1)

        # Extract conditional fields
        offset = 5
        if (flags & cls.FLAG_TIME_STAMP) != 0:
            timeStamp = DateTime.decode(buffer, idx + offset)
            offset += 7
        else:
            timeStamp = DateTime.default()

        if (flags & cls.FLAG_TEMP_TYPE) != 0:
            tempType = buffer[idx + offset]
        else:
            tempType = 0

        return cls(flags, tempMeasurement, timeStamp, tempType)

    def __eq__(self, other):
        if (self.flags != other.flags) or (self.tempMeasurement != other.tempMeasurement):
            return False
        if ((self.flags & self.FLAG_TIME_STAMP) != 0) and (self.timeStamp != other.timeStamp):
            return False
        if ((self.flags & self.FLAG_TEMP_TYPE) != 0) and (self.tempType != other.tempType):
            return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
        if (self.flags & self.FLAG_TIME_STAMP) != 0:
            timeStampStr = "{0}".format(str(self.timeStamp))
        else:
            timeStampStr = ""
        if (self.flags & self.FLAG_TEMP_TYPE) != 0:
            tempTypeStr = "{0}".format(self.userId)
        else:
            tempTypeStr = ""
        return "{0}(flags=0x{1:02X}, tempMeasurement={2}, timeStamp={3}, tempType={4})".format(
                self.__class__.__name__, self.flags, self.tempMeasurement, timeStampStr, tempTypeStr)

    def __str__(self):
        return self.__repr__()

    def __assertFlagValue(self, expected, flag, flagName):
        if (self.flags & flag) != (expected.flags & flag):
            raise ValueError("Unexpected value for {0} (was {1}, expected {2})".format(flagName, (self.flags & flag) != 0, (expected.flags & flag) != 0))

    def assertValue(self, expected, name):
        self.__assertFlagValue(expected, self.FLAG_UNIT_FAHRENHEIT, name + ".flags.FLAG_UNIT_FAHRENHEIT")
        self.__assertFlagValue(expected, self.FLAG_TIME_STAMP, name + ".flags.TIME_STAMP")
        self.__assertFlagValue(expected, self.FLAG_TEMP_TYPE, name + ".flags.FLAG_TEMP_TYPE")
        self.systolic.tempMeasurement(expected.tempMeasurement, name + ".tempMeasurement")
        if (self.flags & self.FLAG_TIME_STAMP) != 0:
            self.timeStamp.assertValue(expected.timeStamp, name + ".timeStamp")
        if (self.flags & self.FLAG_TEMP_TYPE) != 0:
            if self.tempType != expected.tempType:
                raise ValueError("Unexpected value for {0}.tempType (was {1}, expected {2})".format(name, self.tempType, expected.tempType))

    def isUnitFahrenheit(self):
        return (self.flags & self.FLAG_UNIT_FAHRENHEIT) != 0

    def isTimeStampIncluded(self):
        return (self.flags & self.FLAG_TIME_STAMP) != 0

    def isTempTypeIncluded(self):
        return (self.flags & self.FLAG_TEMP_TYPE) != 0
