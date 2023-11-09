/*----------------------------------------------------------------------------

 Copyright 2011, GHJ Morsink


 Author: MorsinkG

   Purpose:


   Contains:

   Module:
      Bridge

------------------------------------------------------------------------------
*/

/***------------------------- Includes ----------------------------------***/
#include <avr/interrupt.h>
#include <stdint.h>
#include "board.h"                    /* system parameters for this board */
#include "serial.h"                     /* Serial connection */
#include "terminal.h"

/***------------------------- Defines ------------------------------------***/


/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

/***------------------------ Global Data --------------------------------***/
uint16_t    uTaskCounter;

/***------------------------ Global functions ---------------------------***/

/*--------------------------------------------------
Sample task
 --------------------------------------------------*/
void vSampleTask1( void )
{
   uTaskCounter += 1;
}

/*--------------------------------------------------
The cooperative RoundRobin
 --------------------------------------------------*/
int main(void)
{
   vInitBoard();                          /* for getting correct internal clock */
   vTerminalInit();
   for (;;)
   {
      vDoTerminal();                    /* testing facility */
      vSampleTask1();                   /* first task */
   }
   return 0;
}

/* EOF */
