#include "pins_arduino.h"

volatile uint8_t *pin_to_mode (uint8_t n)
{
  if (n >= 0 && n < 8)
  {
    return &DDRD;
  }
  else if (n >= 8 && n < 14)
  {
    return &DDRB;
  }
  else if (n >= 14 && n < 19)
  {
    return &DDRC;
  }
  else
  {
    return NOT_A_PIN;
  }
}

volatile uint8_t *pin_to_output (uint8_t n)
{
  if (n >= 0 && n < 8)
  {
    return &PORTD;
  }
  else if (n >= 8 && n < 14)
  {
    return &PORTB;
  }
  else if (n >= 14 && n < 19)
  {
    return &PORTC;
  }
  else
  {
    return NOT_A_PIN;
  }
}

volatile uint8_t *pin_to_input (uint8_t n)
{
  if (n >= 0 && n < 8)
  {
    return &PIND;
  }
  else if (n >= 8 && n < 14)
  {
    return &PINB;
  }
  else if (n >= 14 && n < 19)
  {
    return &PINC;
  }
  else
  {
    return NOT_A_PIN;
  }
}

uint8_t pin_to_bit_mask (uint8_t n)
{
  if (n >= 0 && n < 8)
  {
    return _BV(n);
  }
  else if (n >= 8 && n < 14)
  {
    return _BV(n - 8);
  }
  else if (n >= 14 && n < 19)
  {
    return _BV(n - 14);
  }
  else
  {
    return NOT_A_PIN;
  }
}
