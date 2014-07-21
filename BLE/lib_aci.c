/* Copyright (c) 2014, Nordic Semiconductor ASA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/** @file
  @brief Implementation of the ACI library.
 */

#include <stdlib.h>
#include <string.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "aci.h"
#include "aci_cmds.h"
#include "aci_evts.h"
#include "aci_protocol_defines.h"
#include "acilib.h"
#include "acilib_defs.h"
#include "acilib_if.h"
#include "hal_aci_tl.h"
#include "aci_queue.h"
#include "lib_aci.h"

#define LIB_ACI_DEFAULT_CREDIT_NUMBER   1

static hal_aci_data_t reset_msg = {
  .buffer = {MSG_BASEBAND_RESET_LEN, ACI_CMD_RADIO_RESET}
};

static hal_aci_data_t connect_msg = {
  .buffer = {MSG_CONNECT_LEN, ACI_CMD_CONNECT}
};

static hal_aci_data_t disconnect_msg = {
  .buffer = {MSG_DISCONNECT_LEN, ACI_CMD_DISCONNECT}
};

static hal_aci_data_t send_data_msg = {
  .buffer = {MSG_SEND_DATA_BASE_LEN, ACI_CMD_SEND_DATA}
};

/*
Global additionally used used in aci_setup
*/

bool lib_aci_is_pipe_available(aci_state_t *aci_stat, uint8_t pipe)
{
  return aci_stat->pipes_open_bitmap[pipe/8] & (1 << (pipe % 8));
}

bool lib_aci_radio_reset(void)
{
  return hal_aci_tl_send(&reset_msg);
}

void lib_aci_init(aci_state_t *aci_stat)
{
  uint8_t i;

  for (i = 0; i < PIPES_ARRAY_SIZE; i++)
  {
    aci_stat->pipes_open_bitmap[i]          = 0;
    aci_stat->pipes_closed_bitmap[i]        = 0;
  }

  hal_aci_tl_init(&aci_stat->aci_pins);

  /* Wait for a bit, as the lines may be floating if we got here after a power
   * reset.
   */
  _delay_ms(65);

  /* If RDYN is not low, there is no message pending on the nrF8001, at which
   * point we should performa a radio reset to get in a known state.
   */
  if (!hal_aci_tl_rdyn()) {
    lib_aci_radio_reset();
  }
}

bool lib_aci_connect(uint16_t run_timeout, uint16_t adv_interval)
{
  uint8_t* const buffer = &(connect_msg.buffer[0]);

  *(buffer + OFFSET_ACI_CMD_T_CONNECT +
      OFFSET_ACI_CMD_PARAMS_CONNECT_T_TIMEOUT_MSB) =
    (uint8_t)(run_timeout >> 8);

  *(buffer + OFFSET_ACI_CMD_T_CONNECT +
      OFFSET_ACI_CMD_PARAMS_CONNECT_T_TIMEOUT_LSB) =
    (uint8_t)(run_timeout);

  *(buffer + OFFSET_ACI_CMD_T_CONNECT +
      OFFSET_ACI_CMD_PARAMS_CONNECT_T_ADV_INTERVAL_MSB) =
    (uint8_t)(adv_interval >> 8);

  *(buffer + OFFSET_ACI_CMD_T_CONNECT +
      OFFSET_ACI_CMD_PARAMS_CONNECT_T_ADV_INTERVAL_LSB) =
    (uint8_t)(adv_interval);

  return hal_aci_tl_send(&connect_msg);
}

bool lib_aci_disconnect(aci_state_t *aci_stat, aci_disconnect_reason_t reason)
{
  bool ret_val;
  uint8_t* const buffer = &(disconnect_msg.buffer[0]);

  *(buffer + OFFSET_ACI_CMD_T_DISCONNECT +
      OFFSET_ACI_CMD_PARAMS_DISCONNECT_T_REASON) =
    (uint8_t)(reason);

  ret_val = hal_aci_tl_send(&disconnect_msg);
  /* If we have actually sent the disconnect */
  if (ret_val)
  {
    /* Update pipes immediately so that while the disconnect is happening,
     * the application can't attempt sending another message
     * If the application sends another message before we updated this
     * a ACI Pipe Error Event will be received from nRF8001
     */
    uint8_t i;
    for (i=0; i < PIPES_ARRAY_SIZE; i++)
    {
      aci_stat->pipes_open_bitmap[i] = 0;
      aci_stat->pipes_closed_bitmap[i] = 0;
    }
  }
  return ret_val;
}

bool lib_aci_send_data(uint8_t pipe, uint8_t *p_value, uint8_t size)
{
  uint8_t* const buffer = &(send_data_msg.buffer[0]);

  *(buffer + OFFSET_ACI_CMD_T_LEN) = MSG_SEND_DATA_BASE_LEN + size;

  *(buffer + OFFSET_ACI_CMD_T_SEND_DATA +
      OFFSET_ACI_CMD_PARAMS_SEND_DATA_T_TX_DATA +
      OFFSET_ACI_TX_DATA_T_PIPE_NUMBER) =
    pipe;

  memcpy((buffer + OFFSET_ACI_CMD_T_SEND_DATA +
        OFFSET_ACI_CMD_PARAMS_SEND_DATA_T_TX_DATA +
        OFFSET_ACI_TX_DATA_T_ACI_DATA), p_value, size);

  return hal_aci_tl_send(&send_data_msg);
}

bool lib_aci_event_get(aci_state_t *aci_stat, hal_aci_evt_t *p_aci_evt_data)
{
  bool status = false;

  status = hal_aci_tl_event_get((hal_aci_data_t *)p_aci_evt_data);

  /**
  Update the state of the ACI with the
  ACI Events -> Pipe Status, Disconnected, Connected, Bond Status, Pipe Error
  */
  if (status)
  {
    aci_evt_t * aci_evt;

    aci_evt = &p_aci_evt_data->evt;

    switch(aci_evt->evt_opcode)
    {
        case ACI_EVT_PIPE_STATUS:
            {
                memcpy(aci_stat->pipes_open_bitmap,
                    aci_evt->params.pipe_status.pipes_open_bitmap,
                    PIPES_ARRAY_SIZE);
                memcpy(aci_stat->pipes_closed_bitmap,
                    aci_evt->params.pipe_status.pipes_closed_bitmap,
                    PIPES_ARRAY_SIZE);
            }
            break;

        case ACI_EVT_DISCONNECTED:
            {
                uint8_t i=0;

                for (i=0; i < PIPES_ARRAY_SIZE; i++)
                {
                  aci_stat->pipes_open_bitmap[i] = 0;
                  aci_stat->pipes_closed_bitmap[i] = 0;
                }
                aci_stat->confirmation_pending = false;
                aci_stat->data_credit_available = aci_stat->data_credit_total;

            }
            break;

        case ACI_EVT_TIMING:
                aci_stat->connection_interval = aci_evt->params.timing.conn_rf_interval;
                aci_stat->slave_latency       = aci_evt->params.timing.conn_slave_rf_latency;
                aci_stat->supervision_timeout = aci_evt->params.timing.conn_rf_timeout;
            break;

        default:
            /* Need default case to avoid compiler warnings about missing enum
             * values on some platforms.
             */
            break;
    }
  }
  return status;
}
