#include <string.h>
#include <avr/io.h>
#include <util/delay.h>

#include "../boot.h"

#include "lib_aci.h"
#include "dfu.h"

static void dfu_data_pkt_handle (aci_state_t *aci_state,
    aci_evt_t *aci_evt);
static void dfu_init_pkt_handle (aci_state_t *aci_state,
    aci_evt_t *aci_evt);
static void dfu_image_size_set (aci_state_t *aci_state, aci_evt_t *aci_evt);
static void dfu_image_validate (aci_state_t *aci_state, aci_evt_t *aci_evt);
static void dfu_reset (aci_state_t *aci_state, aci_evt_t *aci_evt);

static void m_notify (aci_state_t *aci_state);
static bool m_send (aci_state_t *aci_state, uint8_t *buff, uint8_t buff_len);
static void m_write_page (uint16_t page, uint8_t *buff);

static uint8_t state = ST_ANY;
static uint32_t firmware_len;
static uint16_t notify_interval;
static uint32_t total_bytes_received;
static uint16_t packets_received;
static uint16_t page;
static uint8_t page_buffer[SPM_PAGESIZE];
static uint8_t page_index;
static uint8_t pipes[3];

/* Send receive notification to the BLE controller */
static void m_notify (aci_state_t *aci_state)
{
  uint8_t response[] = {OP_CODE_PKT_RCPT_NOTIF,
    0,
    (uint8_t) total_bytes_received,
    (uint8_t) (total_bytes_received >> 8),
    (uint8_t) (total_bytes_received >> 16),
    (uint8_t) (total_bytes_received >> 24)};

  m_send (aci_state, response, sizeof(response));
}

/* Transmit buffer_len number of bytes from buffer to the BLE controller */
static bool m_send (aci_state_t *aci_state, uint8_t *buff, uint8_t buff_len)
{
  bool status;

  if (!lib_aci_is_pipe_available (aci_state, pipes[1])) {
    return false;
  }

  if (aci_state->data_credit_available == 0)
  {
    return false;
  }

  status = lib_aci_send_data(pipes[1], buff, buff_len);

  if (status)
  {
    aci_state->data_credit_available--;
  }

  return status;
}

/* Write the contents of buf to the given flash page */
static void m_write_page (uint16_t page_num, uint8_t *buff)
{
  uint16_t i;
  uint16_t word;

  /* Erase flash page, then wait while the memory is written */
  __boot_page_erase_short (page_num);
  boot_spm_busy_wait ();

  for (i = 0; i < SPM_PAGESIZE; i += 2)
  {
      /* Set up little-endian word. */
      word = *buff++;
      word += (*buff++) << 8;

      __boot_page_fill_short (page_num + i, word);
  }

  /* Store buffer in flash page, then wait while the memory is written */
  __boot_page_write_short (page_num);
  boot_spm_busy_wait();

  /* Reenable RWW-section again. We need this if we want to jump back
   * to the application after bootloading. */
  boot_rww_enable ();
}

/*
 * State transition functions
 */

/* Receive a firmware packet, and write it to flash. Also sends receipt
 * notifications if needed
 */
static void dfu_data_pkt_handle (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  static uint8_t response[] = {OP_CODE_RESPONSE,
     BLE_DFU_RECEIVE_APP_PROCEDURE,
     BLE_DFU_RESP_VAL_SUCCESS};

  aci_evt_params_data_received_t *data_received;
  uint8_t bytes_received = aci_evt->len-2;
  uint8_t i;

  /* Send notification for every "notify_interval" number of packets */
  if (0 == (++packets_received % notify_interval))
  {
      m_notify (aci_state);
  }

  /* Write received data to page buffer. When the buffer is full, write the
   * buffer to flash.
   */
  data_received = (aci_evt_params_data_received_t *) &(aci_evt->params);
  for (i = 0; i < bytes_received; i++)
  {
    page_buffer[page_index++] = data_received->rx_data.aci_data[i];

    if (page_index == SPM_PAGESIZE)
    {
      page_index = 0;
      m_write_page (page, page_buffer);
      page += SPM_PAGESIZE;
    }
  }

  /* Check if we've received the entire firmware image */
  total_bytes_received += bytes_received;
  if (firmware_len == total_bytes_received)
  {
    /* Write final page to flash */
    m_write_page (page++, page_buffer);

    /* Send firmware received notification */
    m_send (aci_state, response, sizeof(response));
  }
}

/* Receive and process an init packet */
static void dfu_init_pkt_handle (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  static uint8_t response[] = {OP_CODE_RESPONSE,
     BLE_DFU_INIT_PROCEDURE,
     BLE_DFU_RESP_VAL_SUCCESS};

  /* Send init received notification */
  m_send (aci_state, response, sizeof(response));
}

/* Receive and store the firmware image size */
static void dfu_image_size_set (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  const uint8_t pipe = pipes[1];
  const uint8_t byte_idx = pipe / 8;
  static uint8_t response[] = {OP_CODE_RESPONSE, BLE_DFU_START_PROCEDURE,
    BLE_DFU_RESP_VAL_SUCCESS};

  /* There are two paths into the bootloader. We either got here because there
   * is no application, or we jumped from application. If we jumped from
   * application, we checked that available credit == total credit before the
   * jump, so simply setting it that way is safe.  Further, as we never
   * received an event from the nRF8001 with the pipe statuses, we have to
   * assume that the Control Point TX pipe is open. At this point, that is
   * safe.
   */
  aci_state->pipes_open_bitmap[byte_idx] |= (1 << (pipe % 8));
  aci_state->pipes_closed_bitmap[byte_idx] &= ~(1 << (pipe % 8));

  firmware_len =
    (uint32_t)aci_evt->params.data_received.rx_data.aci_data[3] << 24 |
    (uint32_t)aci_evt->params.data_received.rx_data.aci_data[2] << 16 |
    (uint32_t)aci_evt->params.data_received.rx_data.aci_data[1] << 8  |
    (uint32_t)aci_evt->params.data_received.rx_data.aci_data[0];

  /* Write response */
  m_send (aci_state, response, sizeof(response));

  state = ST_RDY;
}

/* Disconnect from the nRF8001 and do a reset */
static void dfu_reset  (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  hal_aci_evt_t aci_data;

  lib_aci_disconnect(aci_state, ACI_REASON_TERMINATE);

  while(1) {
    if (lib_aci_event_get(aci_state, &aci_data) &&
       (aci_evt->evt_opcode == ACI_EVT_DISCONNECTED)) {
      /* Set watchdog to shortest interval and spin until reset */
      WDTCSR = _BV(WDCE) | _BV(WDE);
      WDTCSR = _BV(WDE);
      while(1);
    }
  }
}

/* Validate the received firmware image, and transmit the result */
static void dfu_image_validate (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  uint8_t response[] = {OP_CODE_RESPONSE,
    BLE_DFU_VALIDATE_PROCEDURE,
    BLE_DFU_RESP_VAL_SUCCESS};

  /* Completed successfully */
  m_send(aci_state, response, sizeof(response));

  state = ST_FW_VALID;
}

/* Update the interval between receipt notifications */
static void dfu_notification_set (aci_state_t *aci_state,
    aci_evt_t *aci_evt)
{
  notify_interval =
    (uint16_t)aci_evt->params.data_received.rx_data.aci_data[2] << 8 |
    (uint16_t)aci_evt->params.data_received.rx_data.aci_data[1];
}

/* Initialize the state machine */
void dfu_init (uint8_t *ppipes)
{
  state = ST_IDLE;
  memcpy(pipes, ppipes, 3);
}

/* Update the state machine according to the event in aci_evt */
void dfu_update (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  const aci_rx_data_t *rx_data = &(aci_evt->params.data_received.rx_data);
  uint8_t event = EV_ANY;
  uint8_t pipe;

  pipe = rx_data->pipe_number;

  /* Incoming data packet */
  if (pipe == pipes[0]) {
    event = DFU_PACKET_RX;
  }
  /* Incoming control point */
  else if (pipe == pipes[2]) {
    event = rx_data->aci_data[0];
  }

  /* Update the state machine */
  switch (event)
  {
    case DFU_PACKET_RX:
      switch (state)
      {
        case ST_IDLE:
          dfu_image_size_set(aci_state, aci_evt);
          break;
        case ST_RX_INIT_PKT:
          dfu_init_pkt_handle(aci_state, aci_evt);
          break;
        case ST_RX_DATA_PKT:
          dfu_data_pkt_handle(aci_state, aci_evt);
          break;
      }
      break;
    case OP_CODE_RECEIVE_INIT:
      if (state == ST_RDY)
        state = ST_RX_INIT_PKT;
      break;
    case OP_CODE_RECEIVE_FW:
      if (state == ST_RDY || state == ST_RX_INIT_PKT)
        state = ST_RX_DATA_PKT;
      break;
    case OP_CODE_VALIDATE:
      if (state == ST_RX_DATA_PKT)
        dfu_image_validate(aci_state, aci_evt);
      break;
    case OP_CODE_ACTIVATE_N_RESET:
      if (state == ST_FW_VALID)
        dfu_reset(aci_state, aci_evt);
      break;
    case OP_CODE_SYS_RESET:
      dfu_reset(aci_state, aci_evt);
      break;
    case OP_CODE_PKT_RCPT_NOTIF_REQ:
      dfu_notification_set (aci_state, aci_evt);
      break;
  }
}
