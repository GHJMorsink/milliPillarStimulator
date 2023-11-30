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
#include <avr/io.h>
#include <stdint.h>
#include <avr/wdt.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "board.h"

/***------------------------- Defines ------------------------------------***/

/*--------------------------------------------------
SPI devices defines
 --------------------------------------------------*/
#define MCP2510_CS         2            /* portpin on which the MCP2515 is selected (PB2) */
/* MCP2510 defines */
#define CANCTRL            0x0F
#define CAN_RESET          0xC0
#define CAN_WRITE          0x02

#define mcp2510_select()   PORTB &= ~(1 << MCP2510_CS)
#define mcp2510_deselect() PORTB |= (1 << MCP2510_CS)

#define MCP42100_CS        2            /* port PB2 selects MCP42100 */
#define COMMAND_P0         0x11         /* command to set potentiometer P0  */
#define COMMAND_P1         0x12         /* command to set potentiometer P1  */
#define COMMAND_P01        0x13         /* command to set potentiometer P0 and P1  */

#define mcp42100_select()   PORTB &= ~(1 << MCP42100_CS)
#define mcp42100_deselect() PORTB |= (1 << MCP42100_CS)

/* The macro version */
#define vXmtSPI(dat)       {SPDR=(dat); loop_until_bit_is_set(SPSR,SPIF);}

/*--------------------------------------------------
channel selects and direction ports for the H-bridges
 port is PD for A, C, D. port is PB for B
 --------------------------------------------------*/
#define A_Enable           2
#define B_Enable           1
#define C_Enable           6
#define D_Enable           4
#define A_Direction        3
#define B_Direction        0
#define C_Direction        7
#define D_Direction        5


#define SetPD(pin)         PORTD |= (1 << pin)
#define ResetPD(pin)       PORTD &= ~(1 << pin)
#define SetPB(pin)         PORTB |= (1 << pin)
#define ResetPB(pin)       PORTB &= ~(1 << pin)

/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

/***------------------------ Global Data --------------------------------***/

/***------------------------ Local functions ----------------------------***/


/*--------------------------------------------------
 Write register to the MCP2510
 --------------------------------------------------*/
#define SetPA(pin)   PORTA |= (1 << pin)
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

   SPCR = 0x50;                        /* Enable SPI function in mode 0 (fast interface) */
   SPSR = 0x00;                        /* SPI normal mode */

   vXmtSPI( CAN_RESET );               /* Reset MCP2515 via spi-interface */
   vMCP2510WriteRegister( CANCTRL, 0x84 );  /* request config mode, clock enable, div by 1 */
   /* we do not need to set the mcp in operational mode! */
}

/*--------------------------------------------------
write data to MCP42100 potentiometer
 --------------------------------------------------*/
static void vWritePot(uint8_t value, uint8_t pot)
{
   uint8_t  command;

   value = (value << 2) + value;        /* value*5 (value is 0..50, pot gets 0..250) */
   switch ( pot )
   {
      case 0 :
         command = COMMAND_P0;
         break;
      case 1 :
         command = COMMAND_P1;
         break;
      default:
         command = COMMAND_P01;
   }
   mcp42100_select();
   vXmtSPI( command );                  /* give command and */
   vXmtSPI( value );                    /* data */
   mcp42100_deselect();
}

/***------------------------ Global functions ---------------------------***/
void vInitBoard(void)
{
   cli();                               /* no interrupts anymore */
   wdt_disable();                       /* disable the watchdog */

   DDRD  = 0xFC;  //0b00101110;        /* Set the port to correct configuration (pd2..pd7 output) */
   PORTD = 0;
   DDRB  = 0x2E;  //0b00101110;        /* Set the port to correct configuration (pb1=led, PB2=CS, PB3=MOSI, PB4=MISO, PB5=CLK */
   PORTB = 0xFF;  //0b11111111;

   vInitMCP();                          /* for getting correct internal clock */
   sei();
   clearHBridge(0);
   clearHBridge(1);
}

/*--------------------------------------------------
 Set a voltage using the pots as DAC
 --------------------------------------------------*/
void setVoltage(uint8_t channel, uint8_t decivolts)
{
   if ( (channel == 0) || (channel == 2) )
   {
      vWritePot(decivolts, 0);
   } else
   {
      vWritePot(decivolts, 1);
   }
}

/*--------------------------------------------------
Set H-Bridge function for a channel
 This will enable the channel in either pos or neg side
 channel: 0 for A and B, 1 for C and D
 --------------------------------------------------*/
void setHBridge(uint8_t channel, uint8_t side)
{
   switch ( channel )
   {
      case 1 :
         if (side != POSITIVE)
         {
            SetPD(A_Direction);
            SetPB(B_Direction);
         } else
         {
            ResetPD(A_Direction);
            ResetPB(B_Direction);
         }
         SetPD(A_Enable);
         SetPB(B_Enable);
         break;
      case 0 :
      default:
         if (side != POSITIVE)
         {
            SetPD(C_Direction);
            SetPD(D_Direction);
         } else
         {
            ResetPD(C_Direction);
            ResetPD(D_Direction);
         }
         SetPD(C_Enable);
         SetPD(D_Enable);
   }
}

/*--------------------------------------------------
 Disable the HBridge channel
 --------------------------------------------------*/
void clearHBridge(uint8_t channel)
{
   switch ( channel )
   {
      case 0 :
         ResetPD(A_Enable);
         ResetPB(B_Enable);
         break;
      case 1 :
      default:
         ResetPD(C_Enable);
         ResetPD(D_Enable);
         break;
   }
}

/* EOF */
