/*  ----------------------------------------------------------------------------
 i2cbang.c
 lightweight bit-banged I2C library for AVR microcontrollers
 -------------------------------------------------------------------------------

   based on:
   http://codinglab.blogspot.com/2008/10/i2c-on-avr-using-bit-banging.html
   by "Raul" http://www.blogger.com/profile/05112542436303049493

   "As a exercise I tried to talk to a I2C temperature sensor using bit
   banging, it was not as easy as I thought so I decided to post the
   code in case anyone needs to see the solution, if you happen to use
   my code drop me a line since that will encourage me to post more
   code :-)" 

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as  as published 
 by the Free Software Foundation http://www.gnu.org/licenses/gpl-2.0.html
 This program is distributed WITHOUT ANY WARRANTY use at your own risk blah blah

   Tidied up and tested with TC74A0 temperature sensor
   by Jonathan Foote jtf@rotormind.com Feb 2013 
*/

#include <avr/io.h>  
#ifndef F_CPU
#define F_CPU 8000000 // 8 MHZ, need to define for the delay below
#endif
#include <util/delay.h>

// Port for the I2C, pins must be on the same port
#define I2C_DDR DDRB
#define I2C_PIN PINB
#define I2C_PORT PORTB

// Pins to be used in the bit banging
// SCLK is PB2 and SDAT is PB0
#define I2C_CLK 2
#define I2C_DAT 0


// I2C but high comes from pullups, so tristate (input) to send high
#define I2C_DATA_HI() I2C_DDR &= ~( 1 << I2C_DAT );
#define I2C_DATA_LO() I2C_DDR |=  ( 1 << I2C_DAT ); 

#define I2C_CLOCK_HI() I2C_DDR &= ~( 1 << I2C_CLK );
#define I2C_CLOCK_LO() I2C_DDR |=  ( 1 << I2C_CLK ); 

void I2C_WriteBit( unsigned char c )
{
  if ( c > 0 ){
    I2C_DATA_HI();
  }
  else {
    I2C_DATA_LO();
  }

  I2C_CLOCK_HI();
  _delay_ms(1);

  I2C_CLOCK_LO();
  _delay_ms(1);

  if ( c > 0 ) 
    I2C_DATA_LO();

  _delay_ms(1);

}

unsigned char I2C_ReadBit()
{
  I2C_DATA_HI();

  I2C_CLOCK_HI();
  _delay_ms(1);

  unsigned char c = I2C_PIN;

  I2C_CLOCK_LO();
  _delay_ms(1);

  return ( c >> I2C_DAT ) & 1;
}

// Inits bitbanging port, must be called before using the functions below
//
void I2C_Init()
{

  /* Drive pins low only */
  I2C_PORT &= ~( ( 1 << I2C_DAT ) | ( 1 << I2C_CLK ) );

  /* Tri-state pins to pull up high */
  I2C_CLOCK_HI();
  I2C_DATA_HI();

  _delay_ms(1);
}

// Send a START Condition
//
void I2C_Start()
{
  // set both to high at the same time
  I2C_DDR &= ~( ( 1 << I2C_DAT ) | ( 1 << I2C_CLK ) );
  _delay_ms(1);

  I2C_DATA_LO();
  _delay_ms(1);

  I2C_CLOCK_LO();
  _delay_ms(1);
}

// Send a STOP Condition
//
void I2C_Stop()
{
  I2C_CLOCK_HI();
  _delay_ms(1);

  I2C_DATA_HI();
  _delay_ms(1);
}

// write a byte to the I2C slave device
//
unsigned char I2C_Write( unsigned char c )
{
  uint8_t i = 0;
  for (i=0;i<8;i++) {
    I2C_WriteBit( c & 128 );
    c<<=1;
  }

  return I2C_ReadBit();
}


// read a byte from the I2C slave device
//
uint8_t I2C_Read(uint8_t ack )
{
  uint8_t res = 0;
  uint8_t i = 0;
  for (i=0;i<8;i++) {
      res <<= 1;
      res |= I2C_ReadBit();
  }

  if ( ack > 0) 
    I2C_WriteBit( 0 );
  else 
    I2C_WriteBit( 1 );
  

  _delay_ms(1);

  return res;
} 

