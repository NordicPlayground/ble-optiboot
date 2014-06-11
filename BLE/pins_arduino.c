#include "pins_arduino.h"

volatile uint8_t *port_to_mode (uint8_t n)
{
  switch(n)
  {
    case 2:
      return &DDRB;
    case 3:
      return &DDRC;
    case 4:
      return &DDRD;
    default:
      return NOT_A_PORT;
  }
}

volatile uint8_t *port_to_output (uint8_t n)
{
  switch(n)
  {
    case 2:
      return &PORTB;
    case 3:
      return &PORTC;
    case 4:
      return &PORTD;
    default:
      return NOT_A_PORT;
  }
}

volatile uint8_t *port_to_input (uint8_t n)
{
  switch(n)
  {
    case 2:
      return &PINB;
    case 3:
      return &PINC;
    case 4:
      return &PIND;
    default:
      return NOT_A_PORT;
  }
}

uint8_t pin_to_port (uint8_t n)
{
  if (n >= 0 && n < 8)
  {
    return PORTD;
  }
  else if (n >= 8 && n < 14)
  {
    return PORTB;
  }
  else if (n >= 14 && n < 19)
  {
    return PORTC;
  }
  else
  {
    return NOT_A_PIN;
  }
}
