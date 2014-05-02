/********************************************************************
* Optiboot bootloader for Arduino                                  *
*                                                                  *
* http://optiboot.googlecode.com                                   *
*                                                                  *
* Arduino-maintained version : See README.TXT                      *
* http://code.google.com/p/arduino/                                *
* It is the intent that changes not relevant to the                *
* Arduino production envionment get moved from the                 *
* optiboot project to the arduino project in "lumps."              *
*                                                                  *
* Heavily optimised bootloader that is faster and                  *
* smaller than the Arduino standard bootloader                     *
*                                                                  *
* Enhancements:                                                    *
* Fits in 512 bytes, saving 1.5K of code space                     *
* Background page erasing speeds up programming                    *
* Higher baud rate speeds up programming                           *
* Written almost entirely in C                                     *
* Customisable timeout with accurate timeconstant                  *
* Optional virtual UART. No hardware UART required.                *
* Optional virtual boot partition for devices without.             *
*                                                                  *
* What you lose:                                                   *
* Implements a skeleton STK500 protocol which is                   *
* missing several features including EEPROM                        *
* programming and non-page-aligned writes                          *
* High baud rate breaks compatibility with standard                *
* Arduino flash settings                                           *
*                                                                  *
* Fully supported:                                                 *
* ATmega168 based devices  (Diecimila etc)                         *
* ATmega328P based devices (Duemilanove etc)                       *
*                                                                  *
* Beta test (believed working.)                                    *
* ATmega8 based devices (Arduino legacy)                           *
* ATmega328 non-picopower devices                                  *
* ATmega644P based devices (Sanguino)                              *
* ATmega1284P based devices                                        *
* ATmega1280 based devices (Arduino Mega)                          *
*                                                                  *
* Alpha test                                                       *
* ATmega32                                                         *
*                                                                  *
* Work in progress:                                                *
* ATtiny84 based devices (Luminet)                                 *
*                                                                  *
* Does not support:                                                *
* USB based devices (eg. Teensy, Leonardo)                         *
*                                                                  *
* Assumptions:                                                     *
* The code makes several assumptions that reduce the               *
* code size. They are all true after a hardware reset,             *
* but may not be true if the bootloader is called by               *
* other means or on other hardware.                                *
* No interrupts can occur                                          *
* UART and Timer 1 are set to their reset state                    *
* SP points to RAMEND                                              *
*                                                                  *
* Code builds on code, libraries and optimisations from:           *
* stk500boot.c                                                     *
* (by Jason P. Kyle)                                               *
* Arduino bootloader                                               *
* (http://arduino.cc)                                              *
* Spiff's 1K bootloader                                            *
* (http://spiffie.org/know/arduino_1k_bootloader/bootloader.shtml) *
* avr-libc project                                                 *
* (http://nongnu.org/avr-libc)                                     *
* Adaboot                                                          *
* (http://www.ladyada.net/library/arduino/bootloader.html)         *
* AVR305                Atmel Application Note                     *
*                                                                  *
* This program is free software; you can redistribute it           *
* and/or modify it under the terms of the GNU General              *
* Public License as published by the Free Software                 *
* Foundation; either version 2 of the License, or                  *
* (at your option) any later version.                              *
*                                                                  *
* This program is distributed in the hope that it will             *
* be useful, but WITHOUT ANY WARRANTY; without even the            *
* implied warranty of MERCHANTABILITY or FITNESS FOR A             *
* PARTICULAR PURPOSE.  See the GNU General Public                  *
* License for more details.                                        *
*                                                                  *
* You should have received a copy of the GNU General               *
* Public License along with this program; if not, write            *
* to the Free Software Foundation, Inc.,                           *
* 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA           *
*                                                                  *
* Licence can be viewed at                                         *
* http://www.fsf.org/licenses/gpl.txt                              *
*                                                                  *
*******************************************************************/

/*******************************************************************
*                                                                  *
* Optional defines:                                                *
*                                                                  *
********************************************************************
*                                                                  *
* BIG_BOOT:                                                        *
* Build a 1k bootloader, not 512 bytes. This turns on              *
* extra functionality.                                             *
*                                                                  *
* BAUD_RATE:                                                       *
* Set bootloader baud rate.                                        *
*                                                                  *
* LUDICROUS_SPEED:                                                 *
* 230400 baud :-)                                                  *
*                                                                  *
* SOFT_UART:                                                       *
* Use AVR305 soft-UART instead of hardware UART.                   *
*                                                                  *
* LED_START_FLASHES:                                               *
* Number of LED flashes on bootup.                                 *
*                                                                  *
* LED_DATA_FLASH:                                                  *
* Flash LED when transferring data. For boards without             *
* TX or RX LEDs, or for people who like blinky lights.             *
*                                                                  *
* SUPPORT_EEPROM:                                                  *
* Support reading and writing from EEPROM. This is not             *
* used by Arduino, so off by default.                              *
*                                                                  *
* TIMEOUT_MS:                                                      *
* Bootloader timeout period, in milliseconds.                      *
* 500,1000,2000,4000,8000 supported.                               *
*                                                                  *
* UART:                                                            *
* UART number (0..n) for devices with more than                    *
* one hardware uart (644P, 1284P, etc)                             *
*                                                                  *
*******************************************************************/

/*******************************************************************
* Version Numbers!                                                 *
*                                                                  *
* Arduino Optiboot now includes this Version number in             *
* the source and object code.                                      *
*                                                                  *
* Version 3 was released as zip from the optiboot                  *
* repository and was distributed with Arduino 0022.                *
* Version 4 starts with the arduino repository commit              *
* that brought the arduino repository up-to-date with              *
* the optiboot source tree changes since v3.                       *
* Version 5 was created at the time of the new Makefile            *
* structure (Mar, 2013), even though no binaries changed           *
* It would be good if versions implemented outside the             *
* official repository used an out-of-seqeunce version              *
* number (like 104.6 if based on based on 4.5) to                  *
* prevent collisions.                                              *
*                                                                  *
*******************************************************************/

/*******************************************************************
* Edit History:                                                    *
*                                                                  *
* Mar 2013                                                         *
* 5.0 WestfW: Major Makefile restructuring.                        *
* See Makefile and pin_defs.h                                      *
* (no binary changes)                                              *
*                                                                  *
* 4.6 WestfW/Pito: Add ATmega32 support                            *
* 4.6 WestfW/radoni: Don't set LED_PIN as an output if             *
* not used. (LED_START_FLASHES = 0)                                *
* Jan 2013                                                         *
* 4.6 WestfW/dkinzer: use autoincrement lpm for read               *
* 4.6 WestfW/dkinzer: pass reset cause to app in R2                *
* Mar 2012                                                         *
* 4.5 WestfW: add infrastructure for non-zero UARTS.               *
* 4.5 WestfW: fix SIGNATURE_2 for m644 (bad in avr-libc)           *
* Jan 2012:                                                        *
* 4.5 WestfW: fix NRWW value for m1284.                            *
* 4.4 WestfW: use attribute OS_main instead of naked for           *
* main().  This allows optimizations that we                       *
* count on, which are prohibited in naked                          *
* functions due to PR42240.  (keeps us less                        *
* than 512 bytes when compiler is gcc4.5                           *
* (code from 4.3.2 remains the same.)                              *
* 4.4 WestfW and Maniacbug:  Add m1284 support.  This              *
* does not change the 328 binary, so the                           *
* version number didn't change either. (?)                         *
* June 2011:                                                       *
* 4.4 WestfW: remove automatic soft_uart detect (didn't            *
* know what it was doing or why.)  Added a                         *
* check of the calculated BRG value instead.                       *
* Version stays 4.4; existing binaries are                         *
* not changed.                                                     *
* 4.4 WestfW: add initialization of address to keep                *
* the compiler happy.  Change SC'ed targets.                       *
* Return the SW version via READ PARAM                             *
* 4.3 WestfW: catch framing errors in getch(), so that             *
* AVRISP works without HW kludges.                                 *
* http://code.google.com/p/arduino/issues/detail?id=368n           *
* 4.2 WestfW: reduce code size, fix timeouts, change               *
* verifySpace to use WDT instead of appstart                       *
* 4.1 WestfW: put version number in binary.                        *
*******************************************************************/

#define OPTIBOOT_MAJVER 5
#define OPTIBOOT_MINVER 0

#define MAKESTR(a) #a
#define MAKEVER(a, b) MAKESTR(a*256+b)

asm("  .section .version\n"
    "optiboot_version:  .word " MAKEVER(OPTIBOOT_MAJVER, OPTIBOOT_MINVER) "\n"
    "  .section .text\n");

#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

/* <avr/boot.h> uses sts instructions, but this version uses out instructions
 * This saves cycles and program memory.
 */
#include "boot.h"
#include "services.h"
#include "lib_aci.h"
#include "aci_evts.h"
#include "dfu.h"


/* We don't use <avr/wdt.h> as those routines have interrupt overhead we don't
 * need.
 */

#include "pin_defs.h"
#include "stk500.h"

#ifndef LED_START_FLASHES
#define LED_START_FLASHES 0
#endif

#ifdef LUDICROUS_SPEED
#define BAUD_RATE 230400L
#endif

/* set the UART baud rate defaults */
#ifndef BAUD_RATE
#if F_CPU >= 8000000L
#define BAUD_RATE   115200L /* Highest rate Avrdude win32 will support */
#elsif F_CPU >= 1000000L
#define BAUD_RATE   9600L   /* 19200 is supported, but with significant error */
#elsif F_CPU >= 128000L
#define BAUD_RATE   4800L   /* Good for 128kHz internal RC */
#else
#define BAUD_RATE 1200L     /* Good even at 32768Hz */
#endif
#endif

#ifndef UART
#define UART 0
#endif

#define BAUD_SETTING (( (F_CPU + BAUD_RATE * 4L) / ((BAUD_RATE * 8L))) - 1 )
#define BAUD_ACTUAL (F_CPU/(8 * ((BAUD_SETTING)+1)))
#define BAUD_ERROR (( 100*(BAUD_RATE - BAUD_ACTUAL) ) / BAUD_RATE)

/*
#if BAUD_ERROR >= 5
#error BAUD_RATE error greater than 5%
#elif BAUD_ERROR <= -5
#error BAUD_RATE error greater than -5%
#elif BAUD_ERROR >= 2
#warning BAUD_RATE error greater than 2%
#elif BAUD_ERROR <= -2
#warning BAUD_RATE error greater than -2%
#endif
*/

#if 0
/* Switch in soft UART for hard baud rates */
/*
 * I don't understand what this was supposed to accomplish, where the
 * constant "280" came from, or why automatically (and perhaps unexpectedly)
 * switching to a soft uart is a good thing, so I'm undoing this in favor
 * of a range check using the same calc used to config the BRG...
 */
#if (F_CPU/BAUD_RATE) > 280 /* > 57600 for 16MHz */
#ifndef SOFT_UART
#define SOFT_UART
#endif
#endif
#else /* 0 */
#if (F_CPU + BAUD_RATE * 4L) / (BAUD_RATE * 8L) - 1 > 250
#error Unachievable baud rate (too slow) BAUD_RATE 
#endif /* baud rate slow check */
#if (F_CPU + BAUD_RATE * 4L) / (BAUD_RATE * 8L) - 1 < 3
#error Unachievable baud rate (too fast) BAUD_RATE 
#endif /* baud rate fastn check */
#endif

/* Watchdog settings */
#define WATCHDOG_OFF    (0)
#define WATCHDOG_16MS   (_BV(WDE))
#define WATCHDOG_32MS   (_BV(WDP0) | _BV(WDE))
#define WATCHDOG_64MS   (_BV(WDP1) | _BV(WDE))
#define WATCHDOG_125MS  (_BV(WDP1) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_250MS  (_BV(WDP2) | _BV(WDE))
#define WATCHDOG_500MS  (_BV(WDP2) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_1S     (_BV(WDP2) | _BV(WDP1) | _BV(WDE))
#define WATCHDOG_2S     (_BV(WDP2) | _BV(WDP1) | _BV(WDP0) | _BV(WDE))
#ifndef __AVR_ATmega8__
#define WATCHDOG_4S     (_BV(WDP3) | _BV(WDE))
#define WATCHDOG_8S     (_BV(WDP3) | _BV(WDP0) | _BV(WDE))
#endif

/* Function Prototypes
 * The main function is in init9, which removes the interrupt vector table
 * we don't need. It is also 'naked', which means the compiler does not
 * generate any entry or exit code itself.
 */
int main(void) __attribute__ ((OS_main)) __attribute__ ((section (".init9")));
static void hardware_init (void);
static void uart_update_firmware (void);
static uint8_t uart_validate_byte (uint8_t ch);
static uint8_t ble_update_firmware(void);
static void putch(uint8_t ch);
static uint8_t getch(void);
static void getNch(uint8_t count);
static void verifySpace();
static void flash_led(uint8_t count);
static inline void watchdogReset();
static void watchdogConfig(uint8_t x);
#ifdef SOFT_UART
static void uartDelay() __attribute__ ((naked));
#endif

/* BLE stuff */
static struct aci_state_t aci_state;

#define BOOTLOADER_KEY 0xDC42
uint16_t boot_key __attribute__((section (".noinit")));
void application_jump_check (void) __attribute__ ((used, naked, section (".init3")));
void application_jump_check (void)
{
  if ((MCUSR & (1 << WDRF)) && (boot_key == BOOTLOADER_KEY)) {
    MCUSR &= ~(1 << WDRF);
    boot_key = 0;
    watchdogConfig(WATCHDOG_OFF);

    ((void (*)(void)) 0x0000)();
  }
}

/*
 * NRWW memory
 * Addresses below NRWW (Non-Read-While-Write) can be programmed while
 * continuing to run code from flash, slightly speeding up programming
 * time.  Beware that Atmel data sheets specify this as a WORD address,
 * while optiboot will be comparing against a 16-bit byte address.  This
 * means that on a part with 128kB of memory, the upper part of the lower
 * 64k will get NRWW processing as well, even though it doesn't need it.
 * That's OK.  In fact, you can disable the overlapping processing for
 * a part entirely by setting NRWWSTART to zero.  This reduces code
 * space a bit, at the expense of being slightly slower, overall.
 *
 * RAMSTART should be self-explanatory.  It's bigger on parts with a
 * lot of peripheral registers.
 */
#if defined(__AVR_ATmega168__)
#define RAMSTART (0x100)
#define NRWWSTART (0x3800)
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega32__)
#define RAMSTART (0x100)
#define NRWWSTART (0x7000)
#elif defined (__AVR_ATmega644P__)
#define RAMSTART (0x100)
#define NRWWSTART (0xE000)
/* correct for a bug in avr-libc */
#undef SIGNATURE_2
#define SIGNATURE_2 0x0A
#elif defined (__AVR_ATmega1284P__)
#define RAMSTART (0x100)
#define NRWWSTART (0xE000)
#elif defined(__AVR_ATtiny84__)
#define RAMSTART (0x100)
#define NRWWSTART (0x0000)
#elif defined(__AVR_ATmega1280__)
#define RAMSTART (0x200)
#define NRWWSTART (0xE000)
#elif defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__)
#define RAMSTART (0x100)
#define NRWWSTART (0x1800)
#endif

/* C zero initialises all global variables. However, that requires */
/* These definitions are NOT zero initialised, but that doesn't matter */
/* This allows us to drop the zero init code, saving us memory */
#define buff    ((uint8_t*)(RAMSTART))
#ifdef VIRTUAL_BOOT_PARTITION
#define rstVect (*(uint16_t*)(RAMSTART+SPM_PAGESIZE*2+4))
#define wdtVect (*(uint16_t*)(RAMSTART+SPM_PAGESIZE*2+6))
#endif

/*
 * Handle devices with up to 4 uarts (eg m1280.)  Rather inelegantly.
 * Note that mega8/m32 still needs special handling, because ubrr is handled
 * differently.
 */
#if UART == 0
# define UART_SRA UCSR0A
# define UART_SRB UCSR0B
# define UART_SRC UCSR0C
# define UART_SRL UBRR0L
# define UART_UDR UDR0
#elif UART == 1
#if !defined(UDR1)
#error UART == 1, but no UART1 on device
#endif
# define UART_SRA UCSR1A
# define UART_SRB UCSR1B
# define UART_SRC UCSR1C
# define UART_SRL UBRR1L
# define UART_UDR UDR1
#elif UART == 2
#if !defined(UDR2)
#error UART == 2, but no UART2 on device
#endif
# define UART_SRA UCSR2A
# define UART_SRB UCSR2B
# define UART_SRC UCSR2C
# define UART_SRL UBRR2L
# define UART_UDR UDR2
#elif UART == 3
#if !defined(UDR1)
#error UART == 3, but no UART3 on device
#endif
# define UART_SRA UCSR3A
# define UART_SRB UCSR3B
# define UART_SRC UCSR3C
# define UART_SRL UBRR3L
# define UART_UDR UDR3
#endif

/* main program starts here */
int main (void)
{
  uint8_t ch;
  hal_aci_evt_t aci_data;

#ifdef SERVICES_PIPE_TYPE_MAPPING_CONTENT
  static services_pipe_type_mapping_t
    services_pipe_type_mapping[NUMBER_OF_PIPES] =
      SERVICES_PIPE_TYPE_MAPPING_CONTENT;
#else
  #define NUMBER_OF_PIPES 0
  static services_pipe_type_mapping_t * services_pipe_type_mapping = NULL;
#endif

  hardware_init ();

  /**
   * Point ACI data structures to the the setup data that the nRFgo studio
   * generated for the nRF8001 */
  if (NULL != services_pipe_type_mapping) {
    aci_state.aci_setup_info.services_pipe_type_mapping =
      &services_pipe_type_mapping[0];
  }
  else {
    aci_state.aci_setup_info.services_pipe_type_mapping = NULL;
  }
  aci_state.aci_setup_info.number_of_pipes = NUMBER_OF_PIPES;
  lib_aci_init (&aci_state);

  boot_key = BOOTLOADER_KEY;

  dfu_initialize();

  for (;;) {
    /* We grab the value in the UDR register without looping, as we need to do
     * a non-blocking read in the event that UART is disabled. This is okay
     * since we validate the data we pull before acting on it. */
    ch = UART_UDR;

    /* Try to get an ACI event from the BLE device. Then, check if the
     * received character is among the STK500 constants, and enter the UART
     * bootloader procedure if it is
    */
    if (ble_update_firmware()) {

      dfu_update(&aci_state, &(aci_data.evt));
      for (;;) {
        (void)ble_update_firmware ();
      }
    } else if (uart_validate_byte (ch)) {
      verifySpace ();
      uart_update_firmware ();
    }
  }
}

static void hardware_init (void)
{
  /* After the zero init loop, this is the first code to run.
   *
   * This code makes the following assumptions:
   *  No interrupts will execute
   *  SP points to RAMEND
   *  r1 contains zero
   *
   * If not, uncomment the following instructions:
   * cli();
   */
  asm volatile ("clr __zero_reg__");
#if defined(__AVR_ATmega8__) || defined (__AVR_ATmega32__)
  SP=RAMEND;  /* This is done by hardware reset */
#endif

  /* Config test line */
  DDRD |= _BV(PD6);
  PORTD &= ~_BV(PD6);

#if LED_START_FLASHES > 0
  /* Set up Timer 1 for timeout counter */
  TCCR1B = _BV(CS12) | _BV(CS10); /* div 1024 */
#endif
#ifndef SOFT_UART
#if defined(__AVR_ATmega8__) || defined (__AVR_ATmega32__)
  UCSRA = _BV(U2X); /* Double speed mode USART */
  UCSRB = _BV(RXEN) | _BV(TXEN);  /* enable Rx & Tx */
  UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);  /* config USART; 8N1 */
  UBRRL = (uint8_t)( (F_CPU + BAUD_RATE * 4L) / (BAUD_RATE * 8L) - 1 );
#else
  UART_SRA = _BV(U2X0); /* Double speed mode USART0 */
  UART_SRB = _BV(RXEN0) | _BV(TXEN0);
  UART_SRC = _BV(UCSZ00) | _BV(UCSZ01);
  UART_SRL = (uint8_t)( (F_CPU + BAUD_RATE * 4L) / (BAUD_RATE * 8L) - 1 );
#endif
#endif

  /* Set up watchdog to trigger after 1000ms */
  watchdogConfig(WATCHDOG_1S);

#if (LED_START_FLASHES > 0) || defined(LED_DATA_FLASH)
  /* Set LED pin as output */
  LED_DDR |= _BV(LED);
#endif

#ifdef SOFT_UART
  /* Set TX pin as output */
  UART_DDR |= _BV(UART_TX_BIT);
#endif

#if LED_START_FLASHES > 0
  /* Flash onboard LED to signal entering of bootloader */
  flash_led(LED_START_FLASHES * 2);
#endif

  /* Reset nRF if the rdyn line is high */
  if (PIND & _BV(PD3)) {
    lib_aci_pin_reset ();
  }
}

static void uart_update_firmware (void)
{
  uint8_t ch;
  uint16_t address;
  uint8_t length;
  unsigned char which;
  uint16_t newAddress;
  uint8_t *bufPtr;
  uint16_t addrPtr;
  uint16_t a;
#ifdef VIRTUAL_BOOT_PARTITION
  uint16_t vect;
#endif

  /* Forever loop */
  for (;;) {
    /* get character from UART */
    ch = getch();

    if(ch == STK_GET_PARAMETER) {
      which = getch();
      verifySpace();
      if (which == 0x82) {
        /*
         * Send optiboot version as "minor SW version"
         */
        putch(OPTIBOOT_MINVER);
      } else if (which == 0x81) {
        putch(OPTIBOOT_MAJVER);
      } else {
        /*
        * GET PARAMETER returns a generic 0x03 reply for
        * other parameters - enough to keep Avrdude happy
        */
        putch(0x03);
      }
    }
    else if(ch == STK_SET_DEVICE) {
      /* SET DEVICE is ignored */
      getNch(20);
    }
    else if(ch == STK_SET_DEVICE_EXT) {
      /* SET DEVICE EXT is ignored */
      getNch(5);
    }
    else if(ch == STK_LOAD_ADDRESS) {
      /* LOAD ADDRESS */
      newAddress = getch();
      newAddress = (newAddress & 0xff) | (getch() << 8);
#ifdef RAMPZ
      /* Transfer top bit to RAMPZ */
      RAMPZ = (newAddress & 0x8000) ? 1 : 0;
#endif
      /* Convert from word address to byte address */
      newAddress += newAddress;
      address = newAddress;
      verifySpace();
    }
    else if(ch == STK_UNIVERSAL) {
      /* UNIVERSAL command is ignored */
      getNch(4);
      putch(0x00);
    }
    /* Write memory, length is big endian and is in bytes */
    else if(ch == STK_PROG_PAGE) {
      /* PROGRAM PAGE - we support flash programming only, not EEPROM */
      getch();
      length = getch();
      getch();

      /* If we are in RWW section, immediately start page erase */
      if (address < NRWWSTART) {
        __boot_page_erase_short((uint16_t)(void*)address);
      }

      /* While that is going on, read in page contents */
      bufPtr = buff;
      do {
        *bufPtr++ = getch();
      } while (--length);

      /* If we are in NRWW section, page erase has to be delayed until now.
       * Todo: Take RAMPZ into account (not doing so just means that we will
       *  treat the top of both "pages" of flash as NRWW, for a slight speed
       *  decrease, so fixing this is not urgent.)
       */
      if (address >= NRWWSTART) {
        __boot_page_erase_short((uint16_t)(void*)address);
      }

      /* Read command terminator, start reply */
      verifySpace();

      /* If only a partial page is to be programmed, the erase might not be
       * complete.  So check that here
       */
      boot_spm_busy_wait();

#ifdef VIRTUAL_BOOT_PARTITION
      if ((uint16_t)(void*)address == 0) {
        /* This is the reset vector page. We need to live-patch the code so the
         * bootloader runs.
         */

        /* Move RESET vector to WDT vector */
        vect = buff[0] | (buff[1]<<8);
        rstVect = vect;
        wdtVect = buff[8] | (buff[9]<<8);
        vect -= 4; /* Instruction is a relative jump (rjmp), so recalculate. */
        buff[8] = vect & 0xff;
        buff[9] = vect >> 8;

        /* Add jump to bootloader at RESET vector */
        buff[0] = 0x7f;
        buff[1] = 0xce; /* rjmp 0x1d00 instruction */
      }
#endif

      /* Copy buffer into programming buffer */
      bufPtr = buff;
      addrPtr = (uint16_t)(void*)address;
      ch = SPM_PAGESIZE / 2;
      do {
        a = *bufPtr++;
        a |= (*bufPtr++) << 8;
        __boot_page_fill_short((uint16_t)(void*)addrPtr,a);
        addrPtr += 2;
      } while (--ch);

      /* Write from programming buffer */
      __boot_page_write_short((uint16_t)(void*)address);
      boot_spm_busy_wait();

#if defined(RWWSRE)
      /* Reenable read access to flash */
      boot_rww_enable();
#endif

    }
    /* Read memory block mode, length is big endian.  */
    else if(ch == STK_READ_PAGE) {
      /* READ PAGE - we only read flash */
      getch();
      length = getch();
      getch();

      verifySpace();
      do {
#ifdef VIRTUAL_BOOT_PARTITION
        /* Undo vector patch in bottom page so verify passes */
        if (address == 0)       ch=rstVect & 0xff;
        else if (address == 1)  ch=rstVect >> 8;
        else if (address == 8)  ch=wdtVect & 0xff;
        else if (address == 9) ch=wdtVect >> 8;
        else ch = pgm_read_byte_near(address);
        address++;
#elif defined(RAMPZ)
        /* Since RAMPZ should already be set, we need to use EPLM directly.
         * Also, we can use the autoincrement version of lpm to update
         * "address" do putch(pgm_read_byte_near(address++)); while (--length);
         * read a Flash and increment the address (may increment RAMPZ)
         */
        __asm__ ("elpm %0,Z+\n" : "=r" (ch), "=z" (address): "1" (address));
#else
        /* read a Flash byte and increment the address */
        __asm__ ("lpm %0,Z+\n" : "=r" (ch), "=z" (address): "1" (address));
#endif
        putch(ch);
      } while (--length);
    }

    /* Get device signature bytes  */
    else if(ch == STK_READ_SIGN) {
      /* READ SIGN - return what Avrdude wants to hear */
      verifySpace();
      putch(SIGNATURE_0);
      putch(SIGNATURE_1);
      putch(SIGNATURE_2);
    }
    else if (ch == STK_LEAVE_PROGMODE) { /* 'Q' */
      /* Adaboot no-wait mod  */
      watchdogConfig(WATCHDOG_16MS);
      verifySpace();
    }
    else {
      /* This covers the response to commands like STK_ENTER_PROGMODE */
      verifySpace();
    }
    putch(STK_OK);
  }
}

static uint8_t uart_validate_byte (uint8_t ch)
{
  uint8_t ret = 0;

  if (ch == STK_OK              ||
      ch == STK_FAILED          ||
      ch == STK_UNKNOWN         ||
      ch == STK_NODEVICE        ||
      ch == ADC_CHANNEL_ERROR   ||
      ch == ADC_MEASURE_OK      ||
      ch == PWM_CHANNEL_ERROR   ||
      ch == PWM_ADJUST_OK       ||
      ch == CRC_EOP             ||
      ch == STK_GET_SYNC        ||
      ch == STK_GET_SIGN_ON     ||
      ch == STK_SET_PARAMETER   ||
      ch == STK_GET_PARAMETER   ||
      ch == STK_SET_DEVICE      ||
      ch == STK_SET_DEVICE_EXT  ||
      ch == STK_ENTER_PROGMODE  ||
      ch == STK_LEAVE_PROGMODE  ||
      ch == STK_CHIP_ERASE      ||
      ch == STK_CHECK_AUTOINC   ||
      ch == STK_LOAD_ADDRESS    ||
      ch == STK_UNIVERSAL       ||
      ch == STK_PROG_FLASH      ||
      ch == STK_PROG_DATA       ||
      ch == STK_PROG_FUSE       ||
      ch == STK_PROG_LOCK       ||
      ch == STK_PROG_PAGE       ||
      ch == STK_PROG_FUSE_EXT   ||
      ch == STK_READ_FLASH      ||
      ch == STK_READ_DATA       ||
      ch == STK_READ_FUSE       ||
      ch == STK_READ_LOCK       ||
      ch == STK_READ_PAGE       ||
      ch == STK_READ_SIGN       ||
      ch == STK_READ_OSCCAL     ||
      ch == STK_READ_FUSE_EXT   ||
      ch == STK_READ_OSCCAL_EXT) {
        ret = 1;
      }

  return ret;
}

static uint8_t ble_update_firmware (void)
{
  hal_aci_evt_t aci_data;
  aci_evt_t *aci_evt;

  uint8_t dfu_mode = 0;

  if (!lib_aci_event_get(&aci_state, &aci_data)) {
    return 0;
  }

  watchdogReset();

  aci_evt = &(aci_data.evt);

  switch(aci_evt->evt_opcode) {
  case ACI_EVT_DEVICE_STARTED:
    aci_state.data_credit_total =
      aci_evt->params.device_started.credit_available;
    if (ACI_DEVICE_STANDBY == aci_evt->params.device_started.device_mode) {
      if (aci_evt->params.device_started.hw_error) {
          /* Magic number used to make sure the HW error event
           * is handled correctly. */
          _delay_ms (20);
      }
      else {
        lib_aci_connect (180,   /* timeout in seconds */
                         0x0050 /* advertising interval 50ms*/);
      }
    }
    break; /* ACI Device Started Event */

  case ACI_EVT_CONNECTED:
    aci_state.data_credit_available = aci_state.data_credit_total;
    break;

  case ACI_EVT_DATA_CREDIT:
    aci_state.data_credit_available = aci_state.data_credit_available +
                                      aci_evt->params.data_credit.credit;
    break;

  case ACI_EVT_PIPE_ERROR:
    if (ACI_STATUS_ERROR_PEER_ATT_ERROR !=
      aci_evt->params.pipe_error.error_code) {
      aci_state.data_credit_available++;
    }
    break;

  case ACI_EVT_DATA_RECEIVED:
    /* If data received is on either of the DFU pipes, return success */
    if (PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_PACKET_RX == aci_evt->params.data_received.rx_data.pipe_number ||
        PIPE_DEVICE_FIRMWARE_UPDATE_BLE_SERVICE_DFU_CONTROL_POINT_RX_ACK_AUTO == aci_evt->params.data_received.rx_data.pipe_number) {
      dfu_mode = 1;
    }

    if (dfu_mode) {
      dfu_update(&aci_state, aci_evt);
    }
    break;

  case ACI_EVT_DISCONNECTED:
    lib_aci_connect (180,   /* timeout in seconds */
                     0x0050 /* advertising interval 50ms*/);
    break;

  case ACI_EVT_HW_ERROR:
    lib_aci_connect (180,   /* timeout in seconds */
                     0x0050 /* advertising interval 50ms*/);
  break;

  case ACI_EVT_PIPE_STATUS:
    break;

  case ACI_EVT_TIMING:
    break;

  default:
    break;
  }

  return dfu_mode;
}

static void putch(uint8_t ch)
{
#ifndef SOFT_UART
  while (!(UART_SRA & _BV(UDRE0)));
  UART_UDR = ch;
#else
  __asm__ __volatile__ (
    "   com %[ch]\n" /* ones complement, carry set */
    "   sec\n"
    "1: brcc 2f\n"
    "   cbi %[uartPort],%[uartBit]\n"
    "   rjmp 3f\n"
    "2: sbi %[uartPort],%[uartBit]\n"
    "   nop\n"
    "3: rcall uartDelay\n"
    "   rcall uartDelay\n"
    "   lsr %[ch]\n"
    "   dec %[bitcnt]\n"
    "   brne 1b\n"
    :
    :
      [bitcnt] "d" (10),
      [ch] "r" (ch),
      [uartPort] "I" (_SFR_IO_ADDR(UART_PORT)),
      [uartBit] "I" (UART_TX_BIT)
    :
      "r25"
  );
#endif
}

static uint8_t getch(void)
{
  uint8_t ch;

#ifdef LED_DATA_FLASH
#if defined(__AVR_ATmega8__) || defined (__AVR_ATmega32__)
  LED_PORT ^= _BV(LED);
#else
  LED_PIN |= _BV(LED);
#endif
#endif

#ifdef SOFT_UART
  __asm__ __volatile__ (
    "1: sbic  %[uartPin],%[uartBit]\n"  /* Wait for start edge */
    "   rjmp  1b\n"
    "   rcall uartDelay\n"              /* Get to middle of start bit */
    "2: rcall uartDelay\n"              /* Wait 1 bit period */
    "   rcall uartDelay\n"              /* Wait 1 bit period */
    "   clc\n"
    "   sbic  %[uartPin],%[uartBit]\n"
    "   sec\n"
    "   dec   %[bitCnt]\n"
    "   breq  3f\n"
    "   ror   %[ch]\n"
    "   rjmp  2b\n"
    "3:\n"
    :
      [ch] "=r" (ch)
    :
      [bitCnt] "d" (9),
      [uartPin] "I" (_SFR_IO_ADDR(UART_PIN)),
      [uartBit] "I" (UART_RX_BIT)
    :
      "r25"
);
#else
  while(!(UART_SRA & _BV(RXC0)))
    ;
  if (!(UART_SRA & _BV(FE0))) {
      /*
       * A Framing Error indicates (probably) that something is talking
       * to us at the wrong bit rate.  Assume that this is because it
       * expects to be talking to the application, and DON'T reset the
       * watchdog.  This should cause the bootloader to abort and run
       * the application "soon", if it keeps happening.  (Note that we
       * don't care that an invalid char is returned...)
       */
    watchdogReset();
  }

  ch = UART_UDR;
#endif

#ifdef LED_DATA_FLASH
#if defined(__AVR_ATmega8__) || defined (__AVR_ATmega32__)
  LED_PORT ^= _BV(LED);
#else
  LED_PIN |= _BV(LED);
#endif
#endif

  return ch;
}

#ifdef SOFT_UART
/* AVR305 equation: #define UART_B_VALUE (((F_CPU/BAUD_RATE)-23)/6) Adding 3 to
 * numerator simulates nearest rounding for more accurate baud rates
 */
#define UART_B_VALUE (((F_CPU/BAUD_RATE)-20)/6)
#if UART_B_VALUE > 255
#error Baud rate too slow for soft UART
#endif

static void uartDelay()
{
  __asm__ __volatile__ (
    "ldi r25,%[count]\n"
    "1:dec r25\n"
    "brne 1b\n"
    "ret\n"
    ::[count] "M" (UART_B_VALUE)
  );
}
#endif

static void getNch(uint8_t count)
{
  do getch(); while (--count);
  verifySpace();
}

static void verifySpace()
{
  if (getch() != CRC_EOP) {
    /* Shorten WD timeout and busy-loop until reset */
    watchdogConfig(WATCHDOG_16MS);
    while (1);
  }
  putch(STK_INSYNC);
}

#if LED_START_FLASHES > 0
static void flash_led(uint8_t count)
{
  do {
    TCNT1 = -(F_CPU/(1024*16));
    TIFR1 = _BV(TOV1);
    while(!(TIFR1 & _BV(TOV1)));
#if defined(__AVR_ATmega8__)  || defined (__AVR_ATmega32__)
    LED_PORT ^= _BV(LED);
#else
    LED_PIN |= _BV(LED);
#endif
    watchdogReset();
  } while (--count);
}
#endif

/* Watchdog functions. These are only safe with interrupts turned off. */
static void watchdogReset()
{
  __asm__ __volatile__ (
    "wdr\n"
  );
}

static void watchdogConfig(uint8_t x)
{
  WDTCSR = _BV(WDCE) | _BV(WDE);
  WDTCSR = x;
}
