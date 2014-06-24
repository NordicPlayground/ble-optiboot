
class DFUOpCodes(object):
    RESERVED_ZERO       = 0
    START_DFU           = 1
    RECEIVE_INIT_PACK   = 2
    RECEIVE_DATA_PACK   = 3
    VALIDATE            = 4
    ACTIVATE_SYS_RESET  = 5
    RESET_SYSTEM        = 6
    REPORT_SIZE         = 7
    PKT_RCPT_NOTIFY_REQ = 8
    RESPONSE_OPCODE     = 16
    PKT_RCPT_NOTIFY_RSP = 17
    RES_1_FUT_MIN       = 9
    RES_1_FUT_MAX       = 15
    RES_2_FUT_MIN       = 18
    RES_2_FUT_MAX       = 255
    def __init__(self):
        pass


class DFUErrCodes(object):
    ErrCodeLookupDict = {}
    RESERVED_ZERO           = 0
    SUCCESS                 = 1
    INVALID_STATE           = 2
    NOT_SUPPORTED           = 3
    DATA_SIZE_EXCEEDS_LIMIT = 4
    CRC_ERROR               = 5
    OPERATION_FAILED        = 6
    RES_FUT_MIN             = 7
    RES_FUT_MAX             = 255
    ErrCodeLookupDict[RESERVED_ZERO]           = 'RESERVED_ZERO'
    ErrCodeLookupDict[SUCCESS]                 = 'SUCCESS'
    ErrCodeLookupDict[INVALID_STATE]           = 'INVALID_STATE'
    ErrCodeLookupDict[NOT_SUPPORTED]           = 'NOT_SUPPORTED'
    ErrCodeLookupDict[DATA_SIZE_EXCEEDS_LIMIT] = 'DATA_SIZE_EXCEEDS_LIMIT'
    ErrCodeLookupDict[CRC_ERROR]               = 'CRC_ERROR'
    def __init__(self):
        pass
