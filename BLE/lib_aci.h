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

#ifndef LIB_ACI_H__
#define LIB_ACI_H__

/** @file
* @brief ACI library
*/

/** @addtogroup lib_aci
@{
@brief Library for the logical part of the Application Controller Interface (ACI)
*/

#include <stdbool.h>

#include "hal_aci_tl.h"
#include "aci_queue.h"
#include "aci.h"
#include "aci_cmds.h"
#include "aci_evts.h"


#define EVT_CMD_RESPONSE_MIN_LENGTH              3

#define PIPES_ARRAY_SIZE                ((ACI_DEVICE_MAX_PIPES + 7)/8)

/* Same size as a hal_aci_data_t */
typedef struct {
  uint8_t   debug_byte;
  aci_evt_t evt;
} _aci_packed_ hal_aci_evt_t;

ACI_ASSERT_SIZE(hal_aci_evt_t, 34);

typedef struct
{
  uint8_t  location; /**< enum aci_pipe_store_t */
  aci_pipe_type_t   pipe_type;
} services_pipe_type_mapping_t;

typedef struct aci_setup_info_t
{
  services_pipe_type_mapping_t *services_pipe_type_mapping;
  uint8_t                       number_of_pipes;
  hal_aci_data_t               *setup_msgs;
  uint8_t                       num_setup_msgs;
} aci_setup_info_t;

typedef struct aci_state_t
{
  /* Pins on the MCU used to connect to the nRF8001 */
  aci_pins_t aci_pins;

  /* Data structures that are created from nRFgo Studio */
  aci_setup_info_t aci_setup_info;

  /* ( aci_bond_status_code_t ) Is the nRF8001 bonded to a peer device */
  uint8_t bonded;

  /* Total data credit available for the specific version of the nRF8001, total
   * equals available when a link is established
  */
  uint8_t data_credit_total;

  /* Operating mode of the nRF8001 */
  aci_device_operation_mode_t device_state;

  /* Start : Variables that are valid only when in a connection */

  /* Available data credits at a specific point of time, ACI_EVT_DATA_CREDIT
   * updates the available credits */
  uint8_t                       data_credit_available;

  /* Multiply by 1.25 to get the connection interval in milliseconds*/
  uint16_t                      connection_interval;

  /* Number of consecutive connection intervals that the nRF8001 is not
   * required to transmit. Use this to save power
  */
  uint16_t                      slave_latency;

  /* Multiply by 10 to get the supervision timeout in milliseconds */
  uint16_t                      supervision_timeout;

  /* Bitmap -> pipes are open and can be used for sending data over the air */
  uint8_t                       pipes_open_bitmap[PIPES_ARRAY_SIZE];

  /* Bitmap -> pipes are closed and cannot be used for sending data over the
   * air
   */
  uint8_t                       pipes_closed_bitmap[PIPES_ARRAY_SIZE];

  /* Attribute protocol Handle Value confirmation is pending for a Handle Value
   * Indication (ACK is pending for a TX_ACK pipe) on local GATT Server
   */
  bool                          confirmation_pending;

  /* End : Variables that are valid only when in a connection */
} aci_state_t;



#define DISCONNECT_REASON_CX_TIMEOUT                 0x08
#define DISCONNECT_REASON_CX_CLOSED_BY_PEER_DEVICE   0x13
#define DISCONNECT_REASON_POWER_LOSS                 0x14
#define DISCONNECT_REASON_CX_CLOSED_BY_LOCAL_DEVICE  0x16
#define DISCONNECT_REASON_ADVERTISER_TIMEOUT         0x50


/** @name Functions for library management */
/* @{ */

/** @brief Initialization function.
 *  @details This function shall be used to initialize/reset ACI Library and
 *    also Resets the nRF8001 by togging the reset pin of the nRF8001. This
 *    function will reset all the variables locally used by ACI library to
 *    their respective default values.
 *  @param bool True if the data was successfully queued for sending,
 *  false if there is no more space to store messages to send.
 */
void lib_aci_init(aci_state_t *aci_stat);

/** @brief Checks if a given pipe is available.
 *  @param pipe Pipe to check.
 *  @return True if the pipe is available, otherwise false.
 */
bool lib_aci_is_pipe_available(aci_state_t *aci_stat, uint8_t pipe);

/* @} */

/** @name ACI Commands available in all modes */
//@{

/** @brief Resets the radio.
 *  @details The function sends a @c BasebandReset command to the radio.
 *  @return True if the transaction is successfully initiated.
 */
bool lib_aci_radio_reset(aci_state_t *aci_stat);

/* @} */

/** @name ACI commands available in Active mode */
/* @{ */

/** @brief Tries to connect to a peer device.
 *  @details This function sends a @c Connect command to the radio.
 *  @param run_timeout Maximum advertising time in seconds (0 means infinite).
 *  @param adv_interval Advertising interval (in multiple of 0.625&nbsp;ms).
 *  @return True if the transaction is successfully initiated.
 */
bool lib_aci_connect(uint16_t run_timeout, uint16_t adv_interval);

/** @brief Disconnects from peer device.
 *  @details This function sends a @c Disconnect command to the radio.
 *  @param reason Reason for disconnecting.
 *  @return True if the transaction is successfully initiated.
 */
bool lib_aci_disconnect(aci_state_t *aci_stat, aci_disconnect_reason_t reason);

/* @} */

/** @name ACI commands available in Connected mode */
/* @{ */

/** @brief Sends data on a given pipe.
 *  @details This function sends a @c SendData command with application data to
 *  the radio. This function memorizes credit use, and checks that
 *  enough credits are available.
 *  @param pipe Pipe number on which the data should be sent.
 *  @param value Pointer to the data to send.
 *  @param size Size of the data to send.
 *  @return True if the transaction is successfully initiated.
 */
bool lib_aci_send_data(uint8_t pipe, uint8_t *value, uint8_t size);

/* @} */

/** @name ACI commands available while connected in Bond mode */
/* @{ */

/* @} */

/** @brief Gets an ACI event from the ACI Event Queue
 *  @details This function gets an ACI event from the ACI event queue.  The
 *    queue is updated by the SPI driver for the ACI running in the interrupt
 *    context
 *  @param aci_stat pointer to the state of the ACI.
 *  @param p_aci_data pointer to the ACI Event. The ACI Event received will be
 *    copied into this pointer.
 *  @return True if an ACI Event was copied to the pointer.
*/
bool lib_aci_event_get(aci_state_t *aci_stat, hal_aci_evt_t * aci_evt);

/* @} */

/* @} */

#endif /* LIB_ACI_H__ */
