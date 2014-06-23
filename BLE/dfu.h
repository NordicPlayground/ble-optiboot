#ifndef DFU_H_
#define DFU_H_

#define ST_IDLE             1
#define ST_RDY              2
#define ST_RX_INIT_PKT      3
#define ST_RX_DATA_PKT      4
#define ST_FW_VALID         5
#define ST_FW_INVALID       6
#define ST_ANY              255

#define DFU_PACKET_RX       20
#define EV_ANY              255

#define OP_CODE_START_DFU             1   /* 'Start DFU' */
#define OP_CODE_RECEIVE_INIT          2   /* 'Initialize DFU parameters' */
#define OP_CODE_RECEIVE_FW            3   /* 'Receive firmware image' */
#define OP_CODE_VALIDATE              4   /* 'Validate firmware' */
#define OP_CODE_ACTIVATE_N_RESET      5   /* 'Activate & Reset' */
#define OP_CODE_SYS_RESET             6   /* 'Reset System' */
#define OP_CODE_IMAGE_SIZE_REQ        7   /* 'Report received image size' .*/
#define OP_CODE_PKT_RCPT_NOTIF_REQ    8   /* 'Request packet rcpt notification.*/
#define OP_CODE_RESPONSE              16  /* 'Response.*/
#define OP_CODE_PKT_RCPT_NOTIF        17   /* 'Packets Receipt Notification'.*/

/**@brief   DFU Procedure type.
 *
 * @details This enumeration contains the types of DFU procedures.
 */
#define BLE_DFU_START_PROCEDURE         1
#define BLE_DFU_INIT_PROCEDURE          2
#define BLE_DFU_RECEIVE_APP_PROCEDURE   3
#define BLE_DFU_VALIDATE_PROCEDURE      4
#define BLE_DFU_PKT_RCPT_REQ_PROCEDURE  8

/**@brief   DFU Response value type.
 */
#define BLE_DFU_RESP_VAL_SUCCESS         1
#define BLE_DFU_RESP_VAL_INVALID_STATE   2
#define BLE_DFU_RESP_VAL_NOT_SUPPORTED   3
#define BLE_DFU_RESP_VAL_DATA_SIZE       4
#define BLE_DFU_RESP_VAL_CRC_ERROR       5
#define BLE_DFU_RESP_VAL_OPER_FAILED     6

void dfu_init (uint8_t *ppipes);
void dfu_update (aci_state_t *aci_state, aci_evt_t *aci_evt);

#endif /* DFU_H_ */
