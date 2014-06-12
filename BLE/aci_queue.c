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
@brief Implementation of a circular queue for ACI data
*/

#include <stdbool.h>
#include <string.h>

#include "hal_aci_tl.h"
#include "aci_queue.h"

void aci_queue_init(aci_queue_t *aci_q)
{
  uint8_t i;

  aci_q->head = 0;
  aci_q->tail = 0;

  for(i=0; i<ACI_QUEUE_SIZE; i++)
  {
    aci_q->aci_data[i].buffer[0] = 0x00;
    aci_q->aci_data[i].buffer[1] = 0x00;
  }
}

bool aci_queue_dequeue(aci_queue_t *aci_q, hal_aci_data_t *p_data)
{
  if (aci_queue_is_empty(aci_q))
  {
    return false;
  }

  memcpy((uint8_t *)p_data, (uint8_t *)&(aci_q->aci_data[aci_q->head]), sizeof(hal_aci_data_t));
  aci_q->head = (aci_q->head + 1) % ACI_QUEUE_SIZE;

  return true;
}

bool aci_queue_enqueue(aci_queue_t *aci_q, hal_aci_data_t *p_data)
{
  const uint8_t length = p_data->buffer[0];

  if (aci_queue_is_full(aci_q))
  {
    return false;
  }

  aci_q->aci_data[aci_q->tail].status_byte = 0;
  memcpy((uint8_t *)&(aci_q->aci_data[aci_q->tail].buffer[0]), (uint8_t *)&p_data->buffer[0], length + 1);
  aci_q->tail = (aci_q->tail + 1) % ACI_QUEUE_SIZE;

  return true;
}

bool aci_queue_is_empty(aci_queue_t *aci_q)
{
  return (aci_q->head == aci_q->tail);
}

bool aci_queue_is_full(aci_queue_t *aci_q)
{
  uint8_t next = (aci_q->tail + 1) % ACI_QUEUE_SIZE;

  return (next == aci_q->head);
}
