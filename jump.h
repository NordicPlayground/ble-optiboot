/* This way of jumping to the bootloader is inspired by bootloaders
 * written by Dean Camera.
 * */
#define BOOTLOADER_KEY 0xDC42
#define BOOTLOADER_EEPROM_SIZE 32

/* application_jump_check is placed in the .init3 section, which means it runs
 * before ordinary C code on reset.
 * We verify that the reset cause was a watchdog reset, and that boot_key
 * has been set, before jumping to the application.
 */
void jump_check (void) __attribute__ ((used, naked, section (".init3")));

/* Clear the boot_key variable */
void jump_boot_key_clear (void);

/* Set the boot_key variable */
void jump_boot_key_set (void);

/* Clear the application valid flag */
void jump_app_key_clear (void);

/* Set the application valid flag */
void jump_app_key_set (void);
