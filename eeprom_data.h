#ifndef EEPROM_DATA_H__
#define EEPROM_DATA_H__

typedef struct
{
  uint8_t reqn_port;
  uint8_t reqn_pin_mask;
  uint8_t rdyn_port;
  uint8_t rdyn_pin_mask;
  uint8_t mosi_port;
  uint8_t mosi_pin_mask;
  uint8_t miso_port;
  uint8_t miso_pin_mask;
  uint8_t sck_port;
  uint8_t sck_pin_mask;
  uint8_t reset_port;
  uint8_t reset_pin_mask;
} eeprom_data_t;


#endif /* EEPROM_DATA_H__ */