/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink

   Purpose:


   Contains:

   Module:
      Stimulator

------------------------------------------------------------------------------
*/

/***------------------------- Includes ----------------------------------***/
#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/wdt.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "serial.h"                     /* Serial connection */

/***------------------------- Defines ------------------------------------***/

#define MCP2510_CS         2            /* portpin on which the MCP2515 is selected (PB2) */

/* MCP2510 defines */
#define CANCTRL            0x0F
#define CAN_RESET          0xC0
#define CAN_WRITE          0x02

#define mcp2510_select()   PORTB &= ~(1 << MCP2510_CS)
#define mcp2510_deselect() PORTB |= (1 << MCP2510_CS)

/* The macro version */
#define vXmtSPI(dat)       {SPDR=(dat); loop_until_bit_is_set(SPSR,SPIF);}


/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

/***------------------------ Global Data --------------------------------***/

/***------------------------ Local functions ----------------------------***/


/*--------------------------------------------------
 Write register to the MCP2510
 --------------------------------------------------*/
static void vMCP2510WriteRegister ( unsigned char uRegister, unsigned char uData )
{
   mcp2510_select();
   vXmtSPI( CAN_WRITE );
   vXmtSPI( uRegister );
   vXmtSPI( uData );
   mcp2510_deselect();
}

/*--------------------------------------------------
 Run the MCP2510 initialisation to get Fclk div 1, 16MHz
 --------------------------------------------------*/
static void vInitMCP( void )
{
   DDRB  = 0x2E;  //0b00101110;        /* Set the port to correct configuration (pb1=led, PB2=CS, PB3=MOSI, PB4=MISO, PB5=CLK */
   PORTB = 0xFF;  //0b11111111;

   SPCR = 0x50;                        /* Enable SPI function in mode 0 (fast interface) */
   SPSR = 0x00;                        /* SPI normal mode */

   vXmtSPI( CAN_RESET );               /* Reset MCP2515 via spi-interface */
   vMCP2510WriteRegister( CANCTRL, 0x84 );  /* request config mode, clock enable, div by 1 */
   /* we do not need to set the mcp in operational mode! */
}

/***------------------------ Global functions ---------------------------***/
void vInitBoard(void)
{
   cli();                               /* no interrupts anymore */
   wdt_disable();                       /* disable the watchdog */
   vInitMCP();                          /* for getting correct internal clock */
   vSerialInit();
   sei();
}

/* EOF */
