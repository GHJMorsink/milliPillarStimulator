/*----------------------------------------------------------------------------

 Copyright 2011, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements serial function

   Contains:

   Module:
      main

------------------------------------------------------------------------------
*/
/***------------------------- Includes ----------------------------------***/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "serial.h"

/***------------------------- Defines -----------------------------------***/

/* The serial buffer takes xx bytes + 2 pointers */
#define SERIAL_RXBUFFERSIZE      128    /* receive buffer size (keep below 256)*/
#define SERIAL_TXBUFFERSIZE      255    /* transmit buffer size (keep below 256)*/
#define MAIN_CLK                 8      /* System runs at 8 MHz */

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/
/* The data is given as flat types; structures give overhead in the generated code */
static uint8_t    acRxBuffer[ SERIAL_RXBUFFERSIZE ];
static uint8_t    iRxInPtr;
static uint8_t    iRxOutPtr;

static uint8_t    acTxBuffer[ SERIAL_TXBUFFERSIZE ];
static uint8_t    iTxInPtr;
static uint8_t    iTxOutPtr;

static uint16_t   const uBaudDetails[7] PROGMEM =
{
   (((MAIN_CLK * 1000000 / 2400) / 8) - 1),
   (((MAIN_CLK * 1000000 / 4800) / 8) - 1),
   (((MAIN_CLK * 1000000 / 9600) / 8) - 1),
   (((MAIN_CLK * 1000000 / 19200) / 8) - 1),
   (((MAIN_CLK * 1000000 / 38400) / 8) - 1),
   (((MAIN_CLK * 1000000 / 57600) / 8) - 1),
   (((MAIN_CLK * 1000000 / 115200L) / 8) - 1)
};

/***------------------------ Global Data --------------------------------***/

uint8_t uRxOverflow;
uint8_t uTxOverflow;

/***------------------------ Local functions ----------------------------***/

/***------------------------ Global functions ---------------------------***/
/*--------------------------------------------------
Baudrate setting
 --------------------------------------------------*/
void vSetBaud( uint8_t uBaudrate )
{
   uint16_t uBaudReg;

   uBaudReg = pgm_read_word( &uBaudDetails[ uBaudrate - 1 ] );
   UBRRH = ( (uBaudReg) >> 8) & 0x0f;
   UBRRL =   (uBaudReg)       & 0xff;
}


/*--------------------------------------------------
 Initialize UART
 --------------------------------------------------*/
void vSerialInit( void )
{
   UCSRA = 2;
   UCSRB = _BV(RXEN) | _BV(RXCIE) | _BV(TXEN);
   UCSRC = _BV(URSEL) | _BV(UCSZ0) | _BV(UCSZ1);

   iRxInPtr = 0;                        /* purge all buffers */
   iRxOutPtr = 0;
   iTxInPtr = 0;
   iTxOutPtr = 0;
   vSetBaud( 3 );          /* set a default baudrate (overwritten by the user) */
   uTxOverflow = 0;
   uRxOverflow = 0;
}

/*--------------------------------------------------
 Get characters non-blocking from the receiver
 --------------------------------------------------*/
uint8_t uSerialGetChar( uint8_t *uRcv )
{
    if ( iRxInPtr != iRxOutPtr )  /* check if something in buffer */
    {
        *uRcv = acRxBuffer[ iRxOutPtr ];
        iRxOutPtr += 1;
        if ( iRxOutPtr == SERIAL_RXBUFFERSIZE )
        {
            iRxOutPtr = 0;
        }
        return RESULT_SUCCESS;                 /* report success */
    }
    return RESULT_ERROR;
}

/*--------------------------------------------------
 Check size of open locations in transmit buffer
 --------------------------------------------------*/
uint8_t uSerialGetFree( void )
{
   if ( iTxInPtr >= iTxOutPtr )
   {
      return (SERIAL_TXBUFFERSIZE - (iTxInPtr - iTxOutPtr));
   }
   return (iTxOutPtr - iTxInPtr);
}


/* Put a character to transmit */
/*--------------------------------------------------
Put a character to transmit; if no room: silently ignore
 --------------------------------------------------*/
void vSerialPutChar(uint8_t uTx)
{
   uint8_t     uNextPtr;

   uNextPtr = iTxInPtr + 1;
   if ( uNextPtr >= SERIAL_TXBUFFERSIZE )
   {
      uNextPtr = 0;
   }
   if ( iTxOutPtr != uNextPtr )         /*  check there is room  */
   {
      acTxBuffer[ iTxInPtr ] = uTx;
      /* Now the next items must be without an tx-out interrupt */
      cli();
      iTxInPtr = uNextPtr;
      UCSRB = _BV(RXEN)|_BV(RXCIE)|_BV(TXEN)|_BV(UDRIE); /* interrupt was off: set it on */
      sei();
   }
   else
   {
      uTxOverflow += 1;                 /* overflow situation */
   }
}

/*--------------------------------------------------
 Receiving interrupt
 --------------------------------------------------*/
ISR(USART_RXC_vect)
{
   uint8_t uNextPtr;

   uNextPtr = iRxInPtr + 1;
   if ( uNextPtr >= SERIAL_RXBUFFERSIZE )
   {
      uNextPtr = 0;
   }
   if ( iRxOutPtr != uNextPtr )         /*  check there is room  */
   {
      acRxBuffer[ iRxInPtr ] = UDR;
      iRxInPtr = uNextPtr;
   }
   else
   {
      uRxOverflow += 1;
   }
}

/*--------------------------------------------------
 Transmitting interrupt
 --------------------------------------------------*/
ISR( USART_UDRE_vect )
{
   if ( iTxInPtr != iTxOutPtr )  /* check if something in buffer */
   {
      UDR = acTxBuffer[ iTxOutPtr ];
      iTxOutPtr += 1;
      if ( iTxOutPtr >= SERIAL_TXBUFFERSIZE )
      {
         iTxOutPtr = 0;
      }
   }
   if ( iTxOutPtr == iTxInPtr )        /* if nothing in the buffer (anymore) */
   {
      UCSRB = _BV(RXEN)|_BV(RXCIE)|_BV(TXEN);  /* switch off the interrupt */
   }
}

/* EOF */
