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
#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/delay.h>

#include "hal_aci_tl.h"
#include "aci_queue.h"
#include "eeprom_data.h"

static void m_aci_event_check (void);
static inline void m_aci_reqn_disable (void);
static inline void m_aci_reqn_enable (void);
static inline void m_aci_q_flush (void);
static bool m_aci_spi_transfer (hal_aci_data_t * data_to_send, hal_aci_data_t * received_data);
static void m_spi_init (void);
static inline uint8_t m_spi_readwrite (const uint8_t aci_byte);

static aci_queue_t    aci_tx_q;
static aci_queue_t    aci_rx_q;
static eeprom_data_t  pins;

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
  if (PIND & _BV(PD3))
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

  /* If there are messages to transmit, and we can store the reply, we request a new transfer */
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

static inline void m_aci_reqn_disable (void)
{
  PORTB |= pins.reqn_pin_mask;
}

static inline void m_aci_reqn_enable (void)
{
  PORTB &= ~pins.reqn_pin_mask;
}

static void m_aci_q_flush(void)
{
  /* Re-initialize aci cmd queue and aci event queue to flush them */
  aci_queue_init(&aci_tx_q);
  aci_queue_init(&aci_rx_q);
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
  /* Configure the IO lines */
  /* Set RDYN as input with pull-up */
  //DDRD &= ~pins.rdyn_pin_mask;
  //PORTD |= pins.rdyn_pin_mask;
  //
  DDRD &= ~_BV(PD3);
  PORTD |= _BV(PD3);

  /* Set REQN, MOSI & SCK as output */
  DDRB |= pins.reqn_pin_mask;
  DDRB |= pins.mosi_pin_mask;
  DDRB |= pins.sck_pin_mask;

  /* Set MISO as input */
  DDRB &= ~pins.miso_pin_mask;

  /* Configure SPI registers */
  SPCR |= _BV(SPE) | _BV(DORD) | _BV(MSTR) | _BV(SPI2X) | _BV(SPR0);
}

static inline uint8_t m_spi_readwrite(const uint8_t aci_byte)
{
  SPDR = aci_byte;
  while(!(SPSR & (1<<SPIF)));
  return SPDR;
}

void hal_aci_tl_init(void)
{
  uint8_t i;
  uint8_t *addr;
  uint8_t n_meta;
  uint8_t *p = (uint8_t *) &pins;

  /* Initialize the ACI Command queue. */
  aci_queue_init(&aci_tx_q);
  aci_queue_init(&aci_rx_q);

  /* Read EEPROM data and put it in memory */
  addr = 0;
  n_meta = eeprom_read_byte (addr);

  addr += n_meta;
  for (i = 0; i < EEPROM_NUM_BYTES; i++) {
    *(p++) = eeprom_read_byte (addr++);
  }

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

void hal_aci_tl_pin_reset(void)
{
  pins.reset_port |= pins.reset_pin_mask;

  PORTD |= pins.reset_pin_mask;
  PORTD &= ~pins.reset_pin_mask;
  PORTD |= pins.reset_pin_mask;

  /* Set the nRF8001 to a known state as required by the data sheet */
  PORTB |= pins.reqn_pin_mask;
  PORTB &= ~pins.miso_pin_mask;
  PORTB &= ~pins.mosi_pin_mask;
  PORTB &= ~pins.sck_pin_mask;

  /* Wait for the nRF8001 to get hold of its lines as the lines float for a few ms after reset */
  _delay_ms(30);
}
