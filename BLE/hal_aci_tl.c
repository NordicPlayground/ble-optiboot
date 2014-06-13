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

#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>

#include "hal_aci_tl.h"
#include "aci_queue.h"
#include "pins_arduino.h"

#define NUM_PIPES 3

static void m_aci_event_check (void);
static void m_aci_pins_set (aci_pins_t *aci_pins);
static inline void m_aci_reqn_disable (void);
static inline void m_aci_reqn_enable (void);
static bool m_aci_spi_transfer (hal_aci_data_t * data_to_send, hal_aci_data_t * received_data);
static void m_spi_init (void);
static uint8_t m_spi_readwrite (const uint8_t aci_byte);

static aci_queue_t    aci_tx_q;
static aci_queue_t    aci_rx_q;

static aci_pins_t *pins;


/*
  Checks the RDYN line and runs the SPI transfer if required.
*/
static void m_aci_event_check(void)
{
  hal_aci_data_t data_to_send;
  hal_aci_data_t received_data;

  /* No room to store incoming messages */
  if (aci_queue_is_full(&aci_rx_q))
  {
    return;
  }

  /* If the ready line is disabled and we have pending messages outgoing we
   * enable the request line
  */
  if (!hal_aci_tl_ready())
  {
    if (!aci_queue_is_empty(&aci_tx_q))
    {
      m_aci_reqn_enable();
    }

    return;
  }

  /* Receive from queue */
  if (!aci_queue_dequeue(&aci_tx_q, &data_to_send))
  {
    /* queue was empty, nothing to send */
    data_to_send.status_byte = 0;
    data_to_send.buffer[0] = 0;
  }

  /* Receive and/or transmit data */
  m_aci_spi_transfer(&data_to_send, &received_data);

  /* If there are messages to transmit, and we can store the reply,
   * we request a new transfer
  */
  if (!aci_queue_is_full(&aci_rx_q) && !aci_queue_is_empty(&aci_tx_q))
  {
    m_aci_reqn_enable();
  }

  /* Check if we received data */
  if (received_data.buffer[0] > 0)
  {
    aci_queue_enqueue(&aci_rx_q, &received_data);
  }

  return;
}

static void m_aci_pins_set (aci_pins_t *aci_pins)
{
  pins = aci_pins;
}

static inline void m_aci_reqn_disable (void)
{
  volatile uint8_t *reqn_out = pin_to_output (pins->reqn_pin);

  *reqn_out |= pin_to_bit_mask(pins->reqn_pin);
}

static inline void m_aci_reqn_enable (void)
{
  volatile uint8_t *reqn_out = pin_to_output (pins->reqn_pin);

  *reqn_out &= ~pin_to_bit_mask(pins->reqn_pin);
}

static bool m_aci_spi_transfer (hal_aci_data_t * data_to_send, hal_aci_data_t * received_data)
{
  uint8_t byte_cnt;
  uint8_t byte_sent_cnt;
  uint8_t max_bytes;

  m_aci_reqn_enable();

  /* Send length, receive header */
  byte_sent_cnt = 0;
  received_data->status_byte = m_spi_readwrite(data_to_send->buffer[byte_sent_cnt++]);
  /* Send first byte, receive length from slave */
  received_data->buffer[0] = m_spi_readwrite(data_to_send->buffer[byte_sent_cnt++]);
  if (0 == data_to_send->buffer[0])
  {
    max_bytes = received_data->buffer[0];
  }
  else
  {
    /* Set the maximum to the biggest size. One command byte is already sent */
    max_bytes = (received_data->buffer[0] > (data_to_send->buffer[0] - 1))
                                          ? received_data->buffer[0]
                                          : (data_to_send->buffer[0] - 1);
  }

  if (max_bytes > HAL_ACI_MAX_LENGTH)
  {
    max_bytes = HAL_ACI_MAX_LENGTH;
  }

  /* Transmit/receive the rest of the packet */
  for (byte_cnt = 0; byte_cnt < max_bytes; byte_cnt++)
  {
    received_data->buffer[byte_cnt+1] =  m_spi_readwrite(data_to_send->buffer[byte_sent_cnt++]);
  }

  /* RDYN should follow the REQN line in approx 100ns */
  m_aci_reqn_disable();

  return (max_bytes > 0);
}

static void m_spi_init (void)
{
  volatile uint8_t *miso_mode = pin_to_mode (pins->miso_pin);
  volatile uint8_t *mosi_mode = pin_to_mode (pins->mosi_pin);
  volatile uint8_t *rdyn_mode = pin_to_mode (pins->rdyn_pin);
  volatile uint8_t *reqn_mode = pin_to_mode (pins->reqn_pin);
  volatile uint8_t *sck_mode = pin_to_mode (pins->sck_pin);

  volatile uint8_t *rdyn_out = pin_to_output (pins->rdyn_pin);

  /* Set MISO as input */
  *miso_mode &= ~pin_to_bit_mask(pins->miso_pin);

  /* Other SPI lines are output */
  *mosi_mode |= pin_to_bit_mask(pins->mosi_pin);
  *reqn_mode |= pin_to_bit_mask(pins->reqn_pin);
  *sck_mode |= pin_to_bit_mask(pins->sck_pin);

  /* RDYN is input with pull-up */
  *rdyn_mode &= ~pin_to_bit_mask(pins->rdyn_pin);
  *rdyn_out |= pin_to_bit_mask(pins->rdyn_pin);

  /* Configure SPI registers */
  SPCR |= pin_to_bit_mask(SPE) | _BV(DORD) | _BV(MSTR) | _BV(SPI2X) | _BV(SPR0);
}

static uint8_t m_spi_readwrite(const uint8_t aci_byte)
{
  SPDR = aci_byte;
  while(!(SPSR & (1<<SPIF)));
  return SPDR;
}

void hal_aci_tl_init(aci_pins_t *aci_pins)
{
  /* Set local pin struct pointer */
  m_aci_pins_set (aci_pins);

  /* Initialize the ACI Command queue. */
  aci_queue_init(&aci_tx_q);
  aci_queue_init(&aci_rx_q);

  /* Set up SPI */
  m_spi_init ();
}

bool hal_aci_tl_send(hal_aci_data_t *p_aci_cmd)
{
  const uint8_t length = p_aci_cmd->buffer[0];
  bool ret_val = false;

  if (length > HAL_ACI_MAX_LENGTH)
  {
    return false;
  }

  ret_val = aci_queue_enqueue(&aci_tx_q, p_aci_cmd);
  if (ret_val)
  {
    if(!aci_queue_is_full(&aci_rx_q))
    {
      /* Lower the REQN only when successfully enqueued */
      m_aci_reqn_enable();
    }
  }

  return ret_val;
}

bool hal_aci_tl_event_get(hal_aci_data_t *p_aci_data)
{
  bool was_full;

  if (!aci_queue_is_full(&aci_rx_q))
  {
    m_aci_event_check();
  }

  was_full = aci_queue_is_full(&aci_rx_q);

  if (aci_queue_dequeue(&aci_rx_q, p_aci_data))
  {
    /* Attempt to pull REQN LOW since we've made room for new messages */
    if (!aci_queue_is_full(&aci_rx_q) && !aci_queue_is_empty(&aci_tx_q))
    {
      m_aci_reqn_enable();
    }

    return true;
  }

  return false;
}

/* Returns true if the ready line is low, or false */
bool hal_aci_tl_ready (void)
{
  volatile uint8_t *rdyn_in = pin_to_input (pins->rdyn_pin);

  return !(*rdyn_in & pin_to_bit_mask(pins->rdyn_pin));
}

void hal_aci_tl_pin_reset(void)
{
  volatile uint8_t *reset_out = pin_to_output (pins->reset_pin);
  volatile uint8_t *reset_mode = pin_to_mode (pins->reset_pin);

  *reset_mode |= pin_to_bit_mask(pins->reset_pin);

  *reset_out |= pin_to_bit_mask(pins->reset_pin);
  *reset_out &= ~pin_to_bit_mask(pins->reset_pin);
  *reset_out |= pin_to_bit_mask(pins->reset_pin);

  /* Set the nRF8001 to a known state as required by the data sheet */
  m_spi_init ();

  /* Wait for the nRF8001 to get hold of its lines as the lines float for a few ms after reset */
  _delay_ms(30);
}
