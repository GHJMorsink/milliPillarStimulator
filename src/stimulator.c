/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink

   Purpose:


   Contains:

   Module:
      Main for the stimulator

------------------------------------------------------------------------------
*/

/***------------------------- Includes ----------------------------------***/
#include <avr/interrupt.h>
#include <stdint.h>
#include "board.h"                    /* system parameters for this board */
#include "serial.h"                   /* Serial connection */
#include "terminal.h"                 /* The command terminal */

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
   vInitBoard();                        /* for getting correct internal clock */
   vTerminalInit();

   for (;;)                             /* The cooperative RoundRobin loop */
   {
      vDoTerminal();                    /* terminal functions */
      vSampleTask1();                   /* waveform generation */
   }
   return 0;
}

/* EOF */
