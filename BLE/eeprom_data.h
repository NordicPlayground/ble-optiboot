#ifndef EEPROM_DATA_H__
#define EEPROM_DATA_H__

#define EEPROM_NUM_BYTES 18

typedef struct
{
  uint16_t reqn_ddr;
  uint16_t reqn_port;
  uint16_t reqn_pin_mask;
  uint16_t rdyn_ddr;
  uint16_t rdyn_port;
  uint16_t rdyn_pin_mask;
  uint16_t mosi_ddr;
  uint16_t mosi_port;
  uint16_t mosi_pin_mask;
  uint16_t miso_ddr;
  uint16_t miso_port;
  uint16_t miso_pin_mask;
  uint16_t sck_ddr;
  uint16_t sck_port;
  uint16_t sck_pin_mask;
  uint16_t reset_ddr;
  uint16_t reset_port;
  uint16_t reset_pin_mask;
} eeprom_data_t;


#endif /* EEPROM_DATA_H__ */
