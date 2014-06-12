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
