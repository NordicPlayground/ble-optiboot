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
#include <util/delay.h>

#include "aci.h"
#include "aci_cmds.h"
#include "aci_evts.h"
#include "aci_protocol_defines.h"
#include "acilib_defs.h"
#include "acilib_if.h"
#include "hal_aci_tl.h"
#include "aci_queue.h"
#include "lib_aci.h"

#define LIB_ACI_DEFAULT_CREDIT_NUMBER   1

/*
Global additionally used used in aci_setup
*/
hal_aci_data_t  msg_to_send;

static services_pipe_type_mapping_t * p_services_pipe_type_map;
static hal_aci_data_t *               p_setup_msgs;

/* The following structure (aci_cmd_params_open_adv_pipe) will be used to store the complete command
 * including the pipes to be opened.
 */
static aci_cmd_params_open_adv_pipe_t aci_cmd_params_open_adv_pipe;

bool lib_aci_is_pipe_available(aci_state_t *aci_stat, uint8_t pipe)
{
  uint8_t byte_idx;

  byte_idx = pipe / 8;
  if (aci_stat->pipes_open_bitmap[byte_idx] & (0x01 << (pipe % 8)))
  {
    return(true);
  }
  return(false);
}

void lib_aci_init(aci_state_t *aci_stat)
{
  uint8_t i;

  for (i = 0; i < PIPES_ARRAY_SIZE; i++)
  {
    aci_stat->pipes_open_bitmap[i]          = 0;
    aci_stat->pipes_closed_bitmap[i]        = 0;
    aci_cmd_params_open_adv_pipe.pipes[i]   = 0;
  }

  is_request_operation_pending     = false;
  is_indicate_operation_pending    = false;
  is_open_remote_pipe_pending      = false;
  is_close_remote_pipe_pending     = false;
  request_operation_pipe           = 0;
  indicate_operation_pipe          = 0;

  p_services_pipe_type_map = aci_stat->aci_setup_info.services_pipe_type_mapping;
  p_setup_msgs             = aci_stat->aci_setup_info.setup_msgs;

  hal_aci_tl_init();
}

bool lib_aci_connect(uint16_t run_timeout, uint16_t adv_interval)
{
  aci_cmd_params_connect_t aci_cmd_params_connect;
  aci_cmd_params_connect.timeout      = run_timeout;
  aci_cmd_params_connect.adv_interval = adv_interval;
  acil_encode_cmd_connect(&(msg_to_send.buffer[0]), &aci_cmd_params_connect);
  return hal_aci_tl_send(&msg_to_send);
}

bool lib_aci_disconnect(aci_state_t *aci_stat, aci_disconnect_reason_t reason)
{
  bool ret_val;
  uint8_t i;
  aci_cmd_params_disconnect_t aci_cmd_params_disconnect;
  aci_cmd_params_disconnect.reason = reason;
  acil_encode_cmd_disconnect(&(msg_to_send.buffer[0]), &aci_cmd_params_disconnect);
  ret_val = hal_aci_tl_send(&msg_to_send);
  /* If we have actually sent the disconnect */
  if (ret_val)
  {
    /* Update pipes immediately so that while the disconnect is happening,
     * the application can't attempt sending another message
     * If the application sends another message before we updated this
     * a ACI Pipe Error Event will be received from nRF8001
     */
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
  aci_cmd_params_send_data_t aci_cmd_params_send_data;


  return hal_aci_tl_send(&msg_to_send);
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

bool lib_aci_ready(void)
{
  return hal_aci_tl_ready ();
}

void lib_aci_pin_reset(void)
{
    hal_aci_tl_pin_reset();
}
