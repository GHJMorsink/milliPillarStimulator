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
#include <avr/eeprom.h>
#include "board.h"                    /* system parameters for this board */
#include "serial.h"                   /* Serial connection */
#include "terminal.h"                 /* The command terminal */
#include "waveform.h"                 /* The pulse generation */
#include "timer.h"

/***------------------------- Defines ------------------------------------***/


/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

/***------------------------ Global Data --------------------------------***/

/***------------------------ Global functions ---------------------------***/


/*--------------------------------------------------
The cooperative RoundRobin
 --------------------------------------------------*/
int main(void)
{
   vInitBoard();                        /* for getting correct internal clock */
   vInitTimer(TWO_MS);
   vSerialInit();
   vTerminalInit();
   vInitWaveform();

   for (;;)                             /* The cooperative RoundRobin loop */
   {
      vDoTerminal();                    /* terminal functions */
      vDoWaveform();                    /* waveform generation */
   }
   return 0;
}

/* EOF */
