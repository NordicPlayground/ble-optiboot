#include <string.h>
#include <avr/io.h>
#include <avr/boot.h>
#include <util/delay.h>

#include "lib_aci.h"
#include "dfu.h"

static uint8_t dfu_data_pkt_handle (aci_state_t *aci_state,
    aci_evt_t *aci_evt);
static uint8_t dfu_init_pkt_handle (aci_state_t *aci_state,
    aci_evt_t *aci_evt);
static uint8_t dfu_image_size_set (aci_state_t *aci_state, aci_evt_t *aci_evt);
static uint8_t dfu_image_validate (aci_state_t *aci_state, aci_evt_t *aci_evt);
static uint8_t dfu_reset (aci_state_t *aci_state, aci_evt_t *aci_evt);

static void m_notify (aci_state_t *aci_state);
static bool m_send (aci_state_t *aci_state, uint8_t *buff, uint8_t buff_len);
static void m_write_page (uint32_t page, uint8_t *buff);

static uint8_t state = ST_ANY;
static uint32_t firmware_len;
static uint16_t notify_interval;
static uint32_t total_bytes_received;
static uint16_t packets_received;
static uint32_t page;
static uint8_t page_buffer[SPM_PAGESIZE];
static uint8_t page_index;

/* DFU command point op codes */
enum
{
    OP_CODE_START_DFU            = 1,   /* 'Start DFU' */
    OP_CODE_RECEIVE_INIT         = 2,   /* 'Initialize DFU parameters' */
    OP_CODE_RECEIVE_FW           = 3,   /* 'Receive firmware image' */
    OP_CODE_VALIDATE             = 4,   /* 'Validate firmware' */
    OP_CODE_ACTIVATE_N_RESET     = 5,   /* 'Activate & Reset' */
    OP_CODE_SYS_RESET            = 6,   /* 'Reset System' */
    OP_CODE_IMAGE_SIZE_REQ       = 7,   /* 'Report received image size' .*/
    OP_CODE_PKT_RCPT_NOTIF_REQ   = 8,   /* 'Request packet rcpt notification.*/
    OP_CODE_RESPONSE             = 16,  /* 'Response.*/
    OP_CODE_PKT_RCPT_NOTIF       = 17   /* 'Packets Receipt Notification'.*/
};

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

  if (!lib_aci_is_pipe_available (aci_state,
      PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_CONTROL_POINT_TX)) {
    return false;
  }

  if (aci_state->data_credit_available == 0)
  {
    return false;
  }

  status = lib_aci_send_data(
        PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_CONTROL_POINT_TX, buff,
        buff_len);

  if (status)
  {
    aci_state->data_credit_available--;
  }

  return status;
}

/* Write the contents of buf to the given flash page */
static void m_write_page (uint32_t page, uint8_t *buff)
{
  uint16_t i;
  uint16_t word;

  /* Erase flash page, then wait while the memory is written */
  boot_page_erase (page);
  boot_spm_busy_wait ();

  for (i = 0; i < SPM_PAGESIZE; i += 2)
  {
      /* Set up little-endian word. */
      word = *buff++;
      word += (*buff++) << 8;

      boot_page_fill (page + i, word);
  }

  /* Store buffer in flash page, then wait while the memory is written */
  boot_page_write (page);
  boot_spm_busy_wait();

  /* Reenable RWW-section again. We need this if we want to jump back
   * to the application after bootloading. */
  boot_rww_enable ();
}

/*
 * State transition functions
 */

/* Transition to the init packet receive state */
static uint8_t dfu_begin_init (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  return ST_RX_INIT_PKT;
}

/* Transition to the data packet receive state */
static uint8_t dfu_begin_transfer (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  return ST_RX_DATA_PKT;
}

/* Receive a firmware packet, and write it to flash. Also sends receipt
 * notifications if needed
 */
static uint8_t dfu_data_pkt_handle (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  static uint8_t response[] = {OP_CODE_RESPONSE,
     BLE_DFU_RECEIVE_APP_PROCEDURE,
     BLE_DFU_RESP_VAL_SUCCESS};

  aci_evt_params_data_received_t *data_received;
  uint8_t bytes_received = aci_evt->len-2;
  uint8_t i;

  /* Write received data to page buffer. When the buffer is full, write it to
   * flash.
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

  /* Send notification for every "notify_interval" number of packets */
  if (0 == (++packets_received % notify_interval))
  {
      m_notify (aci_state);
  }

  /* Check if we've received the entire firmware image */
  total_bytes_received += bytes_received;
  if (firmware_len == total_bytes_received)
  {
    /* Fill the rest of the page buffer */
    for (i = page_index; i < SPM_PAGESIZE; i++)
    {
      page_buffer[page_index++] = 0xFF;
    }

    /* Write final page to flash */
    m_write_page (page++, page_buffer);

    /* Send firmware received notification */
    m_send (aci_state, response, sizeof(response));
  }

  return ST_RX_DATA_PKT;
}

/* Receive and store the firmware image size */
static uint8_t dfu_image_size_set (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  const uint8_t pipe = PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_CONTROL_POINT_TX;
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
  aci_state->data_credit_available = 2;

  firmware_len =
    (uint32_t)aci_evt->params.data_received.rx_data.aci_data[3] << 24 |
    (uint32_t)aci_evt->params.data_received.rx_data.aci_data[2] << 16 |
    (uint32_t)aci_evt->params.data_received.rx_data.aci_data[1] << 8  |
    (uint32_t)aci_evt->params.data_received.rx_data.aci_data[0];

  /* Write response */
  m_send (aci_state, response, sizeof(response));

  return ST_RDY;
}

/* Disconnect from the nRF8001 and do a reset */
static uint8_t dfu_reset  (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  hal_aci_evt_t aci_data;

  lib_aci_disconnect(aci_state, ACI_REASON_TERMINATE);

  while(1) {
    if (lib_aci_event_get(aci_state, &aci_data) &&
       (ACI_EVT_DISCONNECTED == aci_evt->evt_opcode)) {
      /* Set watchdog to shortest interval and spin until reset */
      WDTCSR = _BV(WDCE) | _BV(WDE);
      WDTCSR = _BV(WDE);
      while(1);
    }
  }

  return ST_ANY;
}

/* Process incoming init data. This is not yet implemented, as the Master
 * Control Panel application sends no data.
 */
static uint8_t dfu_init_pkt_handle (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  return ST_RX_INIT_PKT;
}

/* Validate the received firmware image, and transmit the result */
static uint8_t dfu_image_validate (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  uint8_t ret;
  uint8_t response[] = {OP_CODE_RESPONSE,
    BLE_DFU_VALIDATE_PROCEDURE,
    BLE_DFU_RESP_VAL_SUCCESS};

  /* TODO: Implement CRC validation */
  if (total_bytes_received == firmware_len)
  {
    /* Completed successfully */
    m_send(aci_state, response, sizeof(response));

    ret = ST_FW_VALID;
  }
  else
  {
    /* CRC error */
    response[2] = BLE_DFU_RESP_VAL_CRC_ERROR;
    m_send(aci_state, response, sizeof(response));

    ret = ST_FW_INVALID;
  }

  return ret;
}

/* Update the interval between receipt notifications */
static uint8_t dfu_notification_set (aci_state_t *aci_state,
    aci_evt_t *aci_evt)
{
  notify_interval =
    (uint16_t)aci_evt->params.data_received.rx_data.aci_data[2] << 8 |
    (uint16_t)aci_evt->params.data_received.rx_data.aci_data[1];

  return state;
}

/* Table of states, events we should react to in those states, and the
 * transition function that should be called for each defined state/event pair
 */
dfu_transition_t trans[] = {
  { ST_RDY,         OP_CODE_RECEIVE_INIT,            &dfu_begin_init       },
  { ST_IDLE,        DFU_PACKET_RX,                   &dfu_image_size_set   },
  { ST_RX_INIT_PKT, DFU_PACKET_RX,                   &dfu_init_pkt_handle  },
  { ST_RDY,         OP_CODE_RECEIVE_FW,              &dfu_begin_transfer   },
  { ST_RX_INIT_PKT, OP_CODE_RECEIVE_FW,              &dfu_begin_transfer   },
  { ST_RX_DATA_PKT, DFU_PACKET_RX,                   &dfu_data_pkt_handle  },
  { ST_RX_DATA_PKT, OP_CODE_VALIDATE,                &dfu_image_validate   },
  { ST_FW_VALID,    OP_CODE_ACTIVATE_N_RESET,        &dfu_reset            },
  { ST_ANY,         OP_CODE_SYS_RESET,               &dfu_reset            },
  { ST_ANY,         OP_CODE_PKT_RCPT_NOTIF_REQ,      &dfu_notification_set }
};

#define TRANS_COUNT (sizeof(trans)/sizeof(*trans))

/* Initialize the state machine */
void dfu_init (void)
{
  state = ST_IDLE;
}

/* Update the state machine according to the event in aci_evt */
void dfu_update (aci_state_t *aci_state, aci_evt_t *aci_evt)
{
  const aci_rx_data_t *rx_data = &(aci_evt->params.data_received.rx_data);
  uint8_t event = EV_ANY;
  uint8_t i;

  /* Incoming data packet */
  if (PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_PACKET_RX ==
      rx_data->pipe_number)
  {
    event = DFU_PACKET_RX;
  }
  /* Incoming control point */
  else if (
      PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_CONTROL_POINT_RX_ACK_AUTO ==
      rx_data->pipe_number)
  {
    event = rx_data->aci_data[0];
  }

  /* Look through the state machine table and issue a function call
   * if one is stored for the current state and event.
   */
  for (i = 0; i < TRANS_COUNT; i++)
  {
    if (((state == trans[i].st) || (ST_ANY == trans[i].st)) &&
        ((event == trans[i].ev) || (EV_ANY == trans[i].ev)))
    {
      state = (trans[i].fn)(aci_state, aci_evt);
      break;
    }
  }
}
