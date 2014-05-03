#ifndef DFU_H_
#define DFU_H_

#define ST_IDLE             1
#define ST_INIT_ERROR       2
#define ST_RDY              3
#define ST_RX_INIT_PKT      4
#define ST_RX_DATA_PKT      5
#define ST_FW_VALID         6
#define ST_FW_INVALID       7
#define ST_ANY              255

#define DFU_PACKET_RX       20
#define EV_ANY              255

#define PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_PACKET_RX                   8
#define PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_CONTROL_POINT_TX            9
#define PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_CONTROL_POINT_RX_ACK_AUTO   10

/**@brief   DFU Event type.
 *
 * @details This enumeration contains the types of events that will be received
 *  from the DFU Service. See the online DFU documentation for information on
 *  these events
 */
typedef enum
{
    BLE_DFU_START                      = 1,
    BLE_DFU_RECEIVE_INIT_DATA          = 2,
    BLE_DFU_RECEIVE_APP_DATA           = 3,
    BLE_DFU_VALIDATE                   = 4,
    BLE_DFU_ACTIVATE_N_RESET           = 5,
    BLE_DFU_SYS_RESET                  = 6,
    BLE_DFU_PKT_RCPT_NOTIF_ENABLED     = 7,
    BLE_DFU_PKT_RCPT_NOTIF_DISABLED    = 8,
    BLE_DFU_PACKET_WRITE               = 9,
    BLE_DFU_BYTES_RECEIVED_SEND        = 10
} ble_dfu_evt_type_t;

/**@brief   DFU Procedure type.
 *
 * @details This enumeration contains the types of DFU procedures.
 */
typedef enum
{
    BLE_DFU_START_PROCEDURE        = 1,
    BLE_DFU_INIT_PROCEDURE         = 2,
    BLE_DFU_RECEIVE_APP_PROCEDURE  = 3,
    BLE_DFU_VALIDATE_PROCEDURE     = 4,
    BLE_DFU_PKT_RCPT_REQ_PROCEDURE = 8
} ble_dfu_procedure_t;

/**@brief   DFU Response value type.
 */
typedef enum
{
    BLE_DFU_RESP_VAL_SUCCESS        = 1,
    BLE_DFU_RESP_VAL_INVALID_STATE  = 2,
    BLE_DFU_RESP_VAL_NOT_SUPPORTED  = 3,
    BLE_DFU_RESP_VAL_DATA_SIZE      = 4,
    BLE_DFU_RESP_VAL_CRC_ERROR      = 5,
    BLE_DFU_RESP_VAL_OPER_FAILED    = 6
} ble_dfu_resp_val_t;

/**
 *  @brief FSM transition
 *
 * Struct that describes a transition in the FSM.
 */
typedef struct
{   /**
	 * @brief State
	 */
	uint8_t st;

    /**
	 * @brief Event
	 */
	uint8_t ev;

    /**
	 * @brief Transition function
	 */
	uint8_t (*fn)(aci_state_t *aci_state, aci_evt_t *aci_evt);
} dfu_transition_t;

void dfu_initialize (void);
void dfu_update (aci_state_t *aci_state, aci_evt_t *aci_evt);

#endif /* DFU_H_ */
