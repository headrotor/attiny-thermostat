/*  ----------------------------------------------------------------------------
 UART.c
 Routines for interrupt controlled UART
 rewritten for atmegaXX8 from atmega8 by jtf July 2008
 jtf 2008, modified from SWARM code orbswarm.com originally by Pete
 -------------------------------------------------------------------------------

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as  as published 
 by the Free Software Foundation http://www.gnu.org/licenses/gpl-2.0.html
 This program is distributed WITHOUT ANY WARRANTY use at your own risk blah blah 

*/


/* Includes */
#include <avr/io.h>
#include <avr/interrupt.h>
#define BAUD 9600
#define F_CPU 8000000 // 8 KHZ
#include <util/setbaud.h>

#include "UART.h"



/* UART Buffer Defines */
#define UART_RX_BUFFER_SIZE 2     /* 2,4,8,16,32,64,128 or 256 bytes */
#define UART_TX_BUFFER_SIZE 16


#define UART_RX_BUFFER_MASK ( UART_RX_BUFFER_SIZE - 1 )
#if ( UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK )
	#error RX buffer size is not a power of 2
#endif

#define UART_TX_BUFFER_MASK ( UART_TX_BUFFER_SIZE - 1 )
#if ( UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK )
	#error TX buffer size is not a power of 2
#endif


/* Static Variables -- Tx & Rx Ring Buffers */
static unsigned char UART_RxBuf[UART_RX_BUFFER_SIZE];
static volatile unsigned char UART_RxHead;
static volatile unsigned char UART_RxTail;
static unsigned char UART_TxBuf[UART_TX_BUFFER_SIZE];
static volatile unsigned char UART_TxHead;
static volatile unsigned char UART_TxTail;


// -------------------------------------------
// Init UART - Enable Interrupts


void UART_Init(){
  unsigned char x = 0;

  UBRRH = UBRRH_VALUE;
  UBRRL = UBRRL_VALUE;

#if USE_2X
  UCSRA |= (1 << U2X);
#else
  UCSRA &= ~(1 << U2X);
#endif


  /* set to 8 data bits, 1 stop bit */
  UCSRC = (1 << UCSZ1) | (1 << UCSZ0);


  x = 0;										/* Init ring buf indexes */
  UART_RxTail = x;
  UART_RxHead = x;
  UART_TxTail = x;
  UART_TxHead = x;

  /* Enable UART receiver and transmitter */
  UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE)|(1<<UDRIE);
}

// ----------------------------------------------------------------------
// Interrupt handlers - UART Rx vector

SIGNAL(USART_RX_vect){
  unsigned char data;
  unsigned char tmphead;
  
  data = UDR;                 /* Read the received data */
  /* Calculate buffer index */
  tmphead = ( UART_RxHead + 1 ) & UART_RX_BUFFER_MASK;
  UART_RxHead = tmphead;      /* Store new index */
  
  if ( tmphead == UART_RxTail )
    {
      /* ERROR! Receive buffer overflow */
    }
	
  UART_RxBuf[tmphead] = data; /* Store received data in buffer */
}

// ------------------------------------------------------------------------
// Interrupt handler - UART Tx vector for Data Register Empty - UDRE
//SIGNAL(USART_TX_vect)

SIGNAL(USART_UDRE_vect)
{
  unsigned char tmptail;
  
  /* Check if all data is transmitted */
  if ( UART_TxHead != UART_TxTail ) {
    /* Calculate buffer index */
    tmptail = ( UART_TxTail + 1 ) & UART_TX_BUFFER_MASK;
    UART_TxTail = tmptail;      /* Store new index */
    
    UDR = UART_TxBuf[tmptail];  /* Start transmition */
  }
  else {
    UCSRB &= ~(1<<UDRIE);   /* Disable UDRE interrupt or we'll re-trigger on exit */
  }

}

// ---------------------------------------------------------------------
// Check if there are any bytes waiting in the input ring buffer.

unsigned char UART_data_in_ring_buf( void )
{
	return ( UART_RxHead != UART_RxTail ); /* Return 0 (FALSE) if the receive buffer is empty */
}

// -------------------------------------------------------------------------------
// Pull 1 byte from Ring Buffer of bytes received from USART

unsigned char UART_ring_buf_byte( void )
{
	unsigned char tmptail;
	
	while ( UART_RxHead == UART_RxTail )  /* Wait for incoming data */
		;
	tmptail = ( UART_RxTail + 1 ) & UART_RX_BUFFER_MASK;/* Calculate buffer index */
	
	UART_RxTail = tmptail;	/* Store new index */
	return UART_RxBuf[tmptail]; /* Return data */
}

// ----------------------------------------------------

void UART_send_byte( unsigned char data )
{
	unsigned char tmphead;
	/* Calculate buffer index */
	tmphead = ( UART_TxHead + 1 ) & UART_TX_BUFFER_MASK; 

	/* Wait for free space in buffer */
	while ( tmphead == UART_TxTail );

	UART_TxBuf[tmphead] = data; /* Store data in buffer */
	UART_TxHead = tmphead;	    /* Store new index */

	UCSRB |= (1<<UDRIE);	/* Enable UDRE interrupt */
}


