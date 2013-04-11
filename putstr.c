/*  ----------------------------------------------------------------------------
 putstr.c
 Extremely lightweight serial output routines
 jtf 2008, modified from SWARM code orbswarm.com originally by Pete
 -------------------------------------------------------------------------------

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by

 This program is distributed WITHOUT ANY WARRANTY use at your own risk
 blah blah blah

*/


#include <avr/io.h>
#include "UART.h"
#include "putstr.h"

// -------------------------------
// Send a string of chars out the UART port.
// Chrs are put into circular ring buffer.
// Routine returns immediately unless no room in Ring Buf. 
// Interrupts Transmit chrs out of ring buf.

void putstr(char *str)
{
  char ch;
  
  while((ch=*str)!= '\0') {
    UART_send_byte(ch);
    str++;
  }
}

void put_hex(unsigned char number) {
  unsigned char c;
  c = ((number >> 4) & 0x0F);
  if (c > 9)
    UART_send_byte(c -10 + 'A');
  else
    UART_send_byte(c + '0');
  c = number & 0x0F;
  if (c > 9)
    UART_send_byte(c - 10 + 'A');
  else
    UART_send_byte(c + '0');

}




#define DEBUG 1 

#ifdef DEBUG

char value[6]={0,0,0,0,0,0};


void putint(int number)
{

   while((number - 10000)>=0)
  {
    number -= 10000;
    value[5]++;
  }
  value[5] += '0';

  while((number - 1000)>=0)
  {
    number -= 1000;
    value[4]++;
  }
  value[4] += '0';

  while((number - 100)>=0)
  {
    number -= 100;
    value[3]++;
  }
  value[3] += '0';

  while((number - 10)>=0)
  {
    number -= 10;
    value[2]++;
  }
  value[2] += '0';

  value[1] = number + '0';
  value[0] = '\0';

  UART_send_byte(32);	// space
  UART_send_byte(value[5]);
  UART_send_byte(value[4]);
  UART_send_byte(value[3]);
  UART_send_byte(value[2]);
  UART_send_byte(value[1]);
}




// --------------------------------------------------

void putU8(unsigned char number)
{
  char value[3]={0,0,0};

  while((number - 100)>=0)
  {
    number -= 100;
    value[2]++;
  }
  value[2] += '0';

  while((number - 10)>=0)
  {
    number -= 10;
    value[1]++;
  }
  value[1] += '0';

  value[0] = number + '0';

  UART_send_byte(32);	// space
  UART_send_byte(value[2]);
  UART_send_byte(value[1]);
  UART_send_byte(value[0]);
}


void putS16(short number)
{
  char value[6]={0,0,0,0,0,0};

  if(number >= 0)
  {
    value[5]='+';
  }
  else
  {
    value[5]='-';
    number *= -1;
  }

  while((number - 10000)>=0)
  {
    number -= 10000;
    value[4]++;
  }
  value[4] += '0';

  while((number - 1000)>=0)
  {
    number -= 1000;
    value[3]++;
  }
  value[3] += '0';

  while((number - 100)>=0)
  {
    number -= 100;
    value[2]++;
  }
  value[2] += '0';

  while((number - 10)>=0)
  {
    number -= 10;
    value[1]++;
  }
  value[1] += '0';

  value[0] = number + '0';

  UART_send_byte(32);	// space
  UART_send_byte(value[5]);
  UART_send_byte(value[4]);
  UART_send_byte(value[3]);
  UART_send_byte(value[2]);
  UART_send_byte(value[1]);
  UART_send_byte(value[0]);
}

#endif
