/* Copyright (c) 2014, Nordic Semiconductor ASA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * @file
 *
 * @ingroup group_acilib
 *
 * @brief Implementation of the acilib module.
 */

#include <string.h>

#include "aci.h"
#include "aci_cmds.h"
#include "aci_evts.h"
#include "acilib.h"
#include "aci_protocol_defines.h"
#include "acilib_defs.h"
#include "acilib_if.h"
#include "acilib_types.h"


void acil_encode_cmd_connect(uint8_t *buffer,
    aci_cmd_params_connect_t *p_aci_cmd_params_connect)
{
  *(buffer + OFFSET_ACI_CMD_T_LEN) = MSG_CONNECT_LEN;
  *(buffer + OFFSET_ACI_CMD_T_CMD_OPCODE) = ACI_CMD_CONNECT;

  *(buffer + OFFSET_ACI_CMD_T_CONNECT +
      OFFSET_ACI_CMD_PARAMS_CONNECT_T_TIMEOUT_MSB) =
    (uint8_t)(p_aci_cmd_params_connect->timeout >> 8);

  *(buffer + OFFSET_ACI_CMD_T_CONNECT +
      OFFSET_ACI_CMD_PARAMS_CONNECT_T_TIMEOUT_LSB) =
    (uint8_t)(p_aci_cmd_params_connect->timeout);

  *(buffer + OFFSET_ACI_CMD_T_CONNECT +
      OFFSET_ACI_CMD_PARAMS_CONNECT_T_ADV_INTERVAL_MSB) =
    (uint8_t)(p_aci_cmd_params_connect->adv_interval >> 8);

  *(buffer + OFFSET_ACI_CMD_T_CONNECT +
      OFFSET_ACI_CMD_PARAMS_CONNECT_T_ADV_INTERVAL_LSB) =
    (uint8_t)(p_aci_cmd_params_connect->adv_interval);
}

#if 0
void acil_encode_cmd_bond(uint8_t *buffer,
    aci_cmd_params_bond_t *p_aci_cmd_params_bond)
{
  *(buffer + OFFSET_ACI_CMD_T_LEN) = MSG_BOND_LEN;
  *(buffer + OFFSET_ACI_CMD_T_CMD_OPCODE) = ACI_CMD_BOND;

  *(buffer + OFFSET_ACI_CMD_T_BOND + OFFSET_ACI_CMD_PARAMS_BOND_T_TIMEOUT_MSB)
    = (uint8_t)(p_aci_cmd_params_bond->timeout >> 8);

  *(buffer + OFFSET_ACI_CMD_T_BOND + OFFSET_ACI_CMD_PARAMS_BOND_T_TIMEOUT_LSB)
    = (uint8_t)(p_aci_cmd_params_bond->timeout);

  *(buffer + OFFSET_ACI_CMD_T_BOND +
      OFFSET_ACI_CMD_PARAMS_BOND_T_ADV_INTERVAL_MSB) =
    (uint8_t)(p_aci_cmd_params_bond->adv_interval >> 8);

  *(buffer + OFFSET_ACI_CMD_T_BOND +
      OFFSET_ACI_CMD_PARAMS_BOND_T_ADV_INTERVAL_LSB) =
    (uint8_t)(p_aci_cmd_params_bond->adv_interval);
}
#endif

void acil_encode_cmd_disconnect(uint8_t *buffer,
    aci_cmd_params_disconnect_t *p_aci_cmd_params_disconnect)
{
  *(buffer + OFFSET_ACI_CMD_T_LEN) = MSG_DISCONNECT_LEN;
  *(buffer + OFFSET_ACI_CMD_T_CMD_OPCODE) = ACI_CMD_DISCONNECT;

  *(buffer + OFFSET_ACI_CMD_T_DISCONNECT +
      OFFSET_ACI_CMD_PARAMS_DISCONNECT_T_REASON) =
    (uint8_t)(p_aci_cmd_params_disconnect->reason);
}

void acil_encode_cmd_send_data(uint8_t *buffer,
    aci_cmd_params_send_data_t *p_aci_cmd_params_send_data_t,
    uint8_t data_size)
{
  *(buffer + OFFSET_ACI_CMD_T_LEN) = MSG_SEND_DATA_BASE_LEN + data_size;
  *(buffer + OFFSET_ACI_CMD_T_CMD_OPCODE) = ACI_CMD_SEND_DATA;

  *(buffer + OFFSET_ACI_CMD_T_SEND_DATA +
      OFFSET_ACI_CMD_PARAMS_SEND_DATA_T_TX_DATA +
      OFFSET_ACI_TX_DATA_T_PIPE_NUMBER) =
    p_aci_cmd_params_send_data_t->tx_data.pipe_number;

  memcpy((buffer + OFFSET_ACI_CMD_T_SEND_DATA +
        OFFSET_ACI_CMD_PARAMS_SEND_DATA_T_TX_DATA +
        OFFSET_ACI_TX_DATA_T_ACI_DATA),
      &(p_aci_cmd_params_send_data_t->tx_data.aci_data[0]), data_size);
}

#if 0
void acil_encode_cmd_write_dynamic_data(uint8_t *buffer, uint8_t seq_no,
    uint8_t* dynamic_data, uint8_t dynamic_data_size)
{
  *(buffer + OFFSET_ACI_CMD_T_LEN) = MSG_WRITE_DYNAMIC_DATA_BASE_LEN +
    dynamic_data_size;

  *(buffer + OFFSET_ACI_CMD_T_CMD_OPCODE) = ACI_CMD_WRITE_DYNAMIC_DATA;

  *(buffer + OFFSET_ACI_CMD_T_WRITE_DYNAMIC_DATA +
      OFFSET_ACI_CMD_PARAMS_WRITE_DYNAMIC_DATA_T_SEQ_NO) = seq_no;

  memcpy((buffer + OFFSET_ACI_CMD_T_WRITE_DYNAMIC_DATA +
        OFFSET_ACI_CMD_PARAMS_WRITE_DYNAMIC_DATA_T_DYNAMIC_DATA), dynamic_data,
      dynamic_data_size);
}
#endif

#if 0
void acil_encode_cmd_bond_security_request(uint8_t *buffer)
{
  *(buffer + OFFSET_ACI_CMD_T_LEN) = 1;
  *(buffer + OFFSET_ACI_CMD_T_CMD_OPCODE) = ACI_CMD_BOND_SECURITY_REQUEST;
}
#endif

#if 0
void acil_encode_cmd_set_key(uint8_t *buffer,
    aci_cmd_params_set_key_t *p_aci_cmd_params_set_key)
{
  /*
  The length of the key is computed based on the type of key transaction.
  - Key Reject
  - Key type is passkey
  */
  uint8_t len;

  switch (p_aci_cmd_params_set_key->key_type)
  {
    case ACI_KEY_TYPE_INVALID:
    len = MSG_SET_KEY_REJECT_LEN;
    break;
    case ACI_KEY_TYPE_PASSKEY:
    len = MSG_SET_KEY_PASSKEY_LEN;
    break;
    default:
    len=0;
    break;
  }
  *(buffer + OFFSET_ACI_CMD_T_LEN) = len;
  *(buffer + OFFSET_ACI_CMD_T_CMD_OPCODE) = ACI_CMD_SET_KEY;

  *(buffer + OFFSET_ACI_CMD_T_SET_KEY +
      OFFSET_ACI_CMD_PARAMS_SET_KEY_T_KEY_TYPE) =
    p_aci_cmd_params_set_key->key_type;

  /* Use len-2 for the opcode byte and type byte */
  memcpy((buffer + OFFSET_ACI_CMD_T_SET_KEY +
        OFFSET_ACI_CMD_PARAMS_SET_KEY_T_PASSKEY),
      (uint8_t *)&(p_aci_cmd_params_set_key->key), len-2);
}
#endif
