#include "jump.h"

#include <avr/wdt.h>
#include <avr/eeprom.h>

uint16_t boot_key __attribute__((section (".noinit")));
const uint8_t *valid_app_addr = (uint8_t *) 0;

void jump_check (void)
{
  if ((MCUSR & (1 << WDRF)) &&
      (boot_key == BOOTLOADER_KEY))
  {
    /* Clear watchdog reset flag and the boot key */
    MCUSR &= ~(1 << WDRF);
    boot_key = 0;

    /* Disable watchdog */
    WDTCSR = _BV(WDCE) | _BV(WDE);
    WDTCSR = 0;

    /* Jump to application */
    ((void (*)(void)) 0x0000)();
  }
}

void jump_boot_key_clear (void)
{
  boot_key = 0;
}

void jump_boot_key_set (void)
{
  boot_key = BOOTLOADER_KEY;
}
