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
#include <avr/wdt.h>
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
 Watchdog pre-main disable funtion
  --------------------------------------------------*/
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

void get_mcusr(void) __attribute__((naked)) __attribute__((section(".init3")));
void get_mcusr(void)
{
   mcusr_mirror = MCUSR;
   MCUSR = 0;
   wdt_disable();
}


/*--------------------------------------------------
The cooperative RoundRobin
 --------------------------------------------------*/
int main(void)
{
   vInitBoard();                        /* for getting correct internal clock */
   vInitTimer();
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
