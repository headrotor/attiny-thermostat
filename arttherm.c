/*
Title:			arttherm.c

Modified from artcon.c by Windell H. Oskay
Author:	Jonathan Foote jtf@rotormind.com	


Date Created:   2/3/2013
Last Modified:  4/11/2013
 
 Thermostat based on the  Art Controller relay board
 Release version 1.0

Designed to be used with the Art Controller relay board from EMSL
http://www.evilmadscientist.com/2012/artcontroller/
Requires a TC74 temperature sensor connected as follows:

Vdd: PB3
SCK: PB2
GND: PB1
SDA: PB0

In order to reduce wiring, power TC74 from VDD on PB3 and GND on PB1.

PD1 is the TX output of the UART used for debugging. Don't close the
X60 switch or the X10 switch!

More information at http://rotormind.com/blog/2013/Art-Controller-Thermostat
 
Target: Atmel ATtiny2313A MCU
 
Fuse configuration: 
Use 8 MHz internal RC oscillator, with divide-by-8 clock prescaler: 1 MHz clock
BOD active at 2.7 V.
 
Fuses: -U lfuse:w:0x64:m -U hfuse:w:0xdb:m -U efuse:w:0xff:m
 
 -------------------------------------------------
 USAGE: How to compile and install
 
 
 
 A makefile is provided to compile and install this program using AVR-GCC and avrdude.
 
 To use it, follow these steps:
 1. Update the header of the makefile as needed to reflect the type of AVR programmer that you use.
 2. Open a terminal window and move into the directory with this file and the makefile.
 3. At the terminal enter
 make clean   <return>
 make all     <return>
 make install <return>
 4. Make sure that avrdude does not report any errors.  If all goes well, the last few lines output by avrdude
 should look something like this:
 
 avrdude: verifying ...
 avrdude: XXXX bytes of flash verified
 
 avrdude: safemode: lfuse reads as 64
 avrdude: safemode: hfuse reads as DB
 avrdude: safemode: efuse reads as FF
 avrdude: safemode: Fuses OK
 
 avrdude done.  Thank you.
 
 */
 
 /*
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 
 */



#include <avr/io.h>      // device specific I/O definitions
#include <avr/interrupt.h>


// for development, use the UART for a serial port
// for UART, need to specify system clock 
#include "UART.h"
#include "putstr.h"
#include "i2cbang.h"

#define F_CPU 8000000 // 8 MHZ
#include <util/delay.h>


/* buffer for smoothing out temperature reading with running average */
/* buffer size is 2^BUFF_SHIFT in length or 16 for BUFF_SHIFT = 4*/
#define BUFF_SHIFT 4
#define BUFF_SIZE ( 1 << BUFF_SHIFT )
#define BUFF_MASK ( BUFF_SIZE - 1 )
#if ( BUFF_SIZE & BUFF_MASK )
	#error TX buffer size is not a power of 2
#endif


/* to stop annoying chatter, temp must decrease this many
   degrees C below set point before relay is turned off */
#define HYSTERESIS 2

/* Static Variables -- Tx & Rx Ring Buffers */
static uint8_t temp_buff[BUFF_SIZE];
static uint8_t buff_ptr = 0;


/* Coil is controlled by PB4 */
#define TurnCoilOn(); PORTB |= 16;
#define TurnCoilOff(); PORTB &= 239;

/* trigger input is PD6, use to override thermostat */
//#define IS_TRIGGERED (~PIND | 0x64)
#define IS_TRIGGERED ((PIND & _BV(6)) == 0)



// Inputs: PA0, PA1
#define InputMaskA 3

// Inputs: PB0 - PB3
#define InputMaskB 15

// Inputs: PD0 - PD6
#define InputMaskD 127


//stuff for timer which we are not using for thermostat
//volatile variables: ones that may be modified in an ISR
#define MICROSECONDS_PER_COMPARE 128
volatile unsigned int  timer0_microseconds; 
volatile unsigned int timer0_millis; 
volatile unsigned long timer0_seconds; 


/* Get a temperatire reading over I2C from the TC74 sensor */
uint8_t GetTemp(void){
  uint8_t temp;
  I2C_Start();
  I2C_Write( 0x91 );
  temp = I2C_Read( 0x01);
  I2C_Stop();
  return temp;
}


/* to smooth out noisy temperature readings, average over past values */
/* add new temp value to buffer and return running average */
uint8_t BufferTemp(uint8_t temp){
  uint8_t i;
  uint16_t acc = 0; 		/* accumulator for average */

  temp_buff[buff_ptr++] = temp;

  buff_ptr &= BUFF_MASK; 	/* wrap around buff_ptr */
  
  for(i = 0; i < BUFF_SIZE; i++){
    acc += (uint16_t)temp_buff[i];
  }

  // return sum of buffer divided by length
  acc = acc >> BUFF_SHIFT;
  return ((uint8_t) acc & 0xFF);

}


/* Initialize the temperature buffer to a given temperature */
void InitBuffer(uint8_t temp){
  uint8_t i;
  for(i = 0; i < BUFF_SIZE; i++){
    temp_buff[i] = temp;
  }

}

 
uint8_t GetSetPointFromDIP(void)
{
    uint8_t total = 0;
     
    if ((PIND & _BV(5)) == 0)
        total = 1;
    if ((PIND & _BV(4))== 0)
        total += 2;
    if ((PIND & _BV(3)) == 0)
        total += 4;
    if ((PIND & _BV(2)) == 0)
        total += 8;
    if ((PINA & _BV(0)) == 0)
        total += 16;
    if ((PINA & _BV(1))== 0)
        total += 32;
 
    // PD1 used for TX  for debug
   //    if ((PIND & _BV(1))== 0)
    //    total += 64;

    //    if ((PIND & _BV(0))== 0)
    //    temp += 128;

    return total;
}


int main (void)
{ 
    uint8_t setpoint = 0;	/* setpoint in degrees centigrade */
    uint8_t tempc = 0; 		/* temp reading in centigrade */
    
    DDRA = 0;
    DDRB = _BV(4);                 // Set line B4 to be an output, the rest are inputs.
    DDRD = 0;
    
    // Pull-ups on inputs:
    PORTA = InputMaskA;
    PORTB = InputMaskB;
    PORTD = InputMaskD; 
    

    /* We're not really usingthe timer but we'll keep it running... */
    CLKPR = (1 << CLKPCE);        // enable clock prescaler update
    CLKPR = 0;                    // set clock to maximum
    WDTCSR = 0x00;                // disable watchdog timer
    // Set up the timer interrupt:
    TCCR0A = 2;
    OCR0A = 128;
    TIFR = (1 << TOV0);           // clear interrupt flag
    TIMSK = (1 << OCIE0A);         // enable compare interrupt
    TCCR0B = (1 << CS01);         // start timer, prescale by 8

    /* set up the uart for serial debugging; baud rate and sys clock
     are set in UART.c. TX and RX are on X10 and X60 (PD0 PD1)
     switches, keep them OFF. UART rate set in */
    UART_Init();



    /* set up the bit-banged I2C channel to get temp readings, SDA and SCL
     pin assignments are in I2Cbang.c*/
    I2C_Init();

    // Pull-ups on inputs:
    PORTA = 0x03;
    DDRA &= ~(0x03);
    
    /* Sneaky here, in order to reduce wiring, power TC74 from VDD on
     PB3 and GND on PB1. SCLK is PB2 and SDAT is PB0, but those are
     set in i2cbang.c  Also drive PB7, PB6, PB5*/


    DDRB |=  0b11101010;
    PORTB |= 0b00001000;
    PORTB &= 0b11111101;
    //PORTB = InputMaskB;
    PORTD = InputMaskD; 

    sei();		// ENABLE global interrupts


    putstr("Therm 1.0\r\n");

    TurnCoilOff();

    /* like making waffles, throw the first batch away */
    GetTemp();
    _delay_ms(10);		

    GetTemp();
    _delay_ms(10);		


    /* preload the average buffer with the current temp */
    InitBuffer(GetTemp());
         
    for (;;) {  // main loop
        
      setpoint = GetSetPointFromDIP();

      tempc = GetTemp();

      putstr("\r\nsp: ");
      putU8(setpoint);


      putstr(" tc: ");
      putU8(tempc);
      /* add temp to running average, and get the average back */
      tempc = BufferTemp(tempc);
      putstr(" bt: ");
      putU8(tempc);

      putstr(" trig: ");
      putU8((uint8_t)IS_TRIGGERED);


      /* TRIGGER IN is thermostat override switch on PD6*/
      if ((tempc > setpoint) || IS_TRIGGERED) {
	TurnCoilOn();
      }
      else if (tempc <= setpoint - HYSTERESIS) {
	TurnCoilOff();
      }

      /* wait this many milliseconds */
      _delay_ms(500);		



    }
    return 0;
}

SIGNAL (TIMER0_COMPA_vect) {  
  timer0_microseconds += MICROSECONDS_PER_COMPARE;
  if (timer0_microseconds > 1000) {
    timer0_millis++;
    timer0_microseconds -= 1000;
    
    if (timer0_millis > 1000) {
      timer0_seconds++;
      timer0_millis -= 1000;
    }
  } 
    
}
