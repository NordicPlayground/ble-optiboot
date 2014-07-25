#include "bonding.h"

#include <avr/eeprom.h>
#include <util/delay.h>

/*
Temporary buffers for sending ACI commands
*/
static hal_aci_evt_t  aci_data;
static hal_aci_data_t aci_cmd;

/*
Read the Dymamic data from the EEPROM and send then as ACI Write Dynamic Data to the nRF8001
This will restore the nRF8001 to the situation when the Dynamic Data was Read out
*/
aci_status_code_t bond_data_restore(aci_state_t *aci_stat, uint8_t eeprom_status)
{
  aci_evt_t *aci_evt;

  uint16_t eeprom_read_addr = 1;
  uint8_t len = 0;

  uint8_t write_dyn_num_msgs = eeprom_status & 0x7F;

  /* Read from the EEPROM */
  while(1)
  {
    len = eeprom_read_byte ((uint8_t *) eeprom_read_addr);
    eeprom_read_addr++;
    aci_cmd.buffer[0] = len;

    eeprom_read_block ((void *) &aci_cmd.buffer[1], (uint8_t *) eeprom_read_addr, len);
    eeprom_read_addr += len;

    /* Send the ACI Write Dynamic Data */
    if (!hal_aci_tl_send(&aci_cmd))
    {
      return ACI_STATUS_ERROR_INTERNAL;
    }

    /* Spin in the while loop waiting for an event */
    while (1)
    {
      if (lib_aci_event_get(aci_stat, &aci_data))
      {
        aci_evt = &aci_data.evt;

        if (aci_evt->evt_opcode == ACI_EVT_CMD_RSP)
        {
          /* ACI Evt Command Response */
          if (aci_evt->params.cmd_rsp.cmd_status == ACI_STATUS_TRANSACTION_COMPLETE)
          {
            aci_stat->bonded = ACI_BOND_STATUS_SUCCESS;

            return ACI_STATUS_TRANSACTION_COMPLETE;
          }

          /* If the message counter has reached zero,
           * we should have completed by now.
           */
          if (--write_dyn_num_msgs == 0)
          {
            return ACI_STATUS_ERROR_INTERNAL;
          }

          if (aci_evt->params.cmd_rsp.cmd_status == ACI_STATUS_TRANSACTION_CONTINUE)
          {
            /* Break and write the next ACI Write Dynamic Data */
            break;
          }
        }
      }
    }
  }
}
