/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink


   Purpose:
      Implements waveform generation

   Contains:

   Module:

------------------------------------------------------------------------------
*/
#include "waveform.h"
#include <stdint.h>
#include <string.h>
#include <avr/pgmspace.h>

/***------------------------- Defines -----------------------------------***/

/* The serial buffer takes xx bytes + 2 pointers */
#define SERIAL_RXBUFFERSIZE      128    /* receive buffer size (keep below 256)*/
#define SERIAL_TXBUFFERSIZE      255    /* transmit buffer size (keep below 256)*/
#define MAIN_CLK                 8      /* System runs at 8 MHz */

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/
/* The data is given as flat types; structures give overhead in the generated code */
static uint8_t    currentState[CHANNELCOUNT];
static uint32_t   currentCount[CHANNELCOUNT];

/***------------------------ Global Data --------------------------------***/

uint8_t  uStartFlag[CHANNELCOUNT] = { 0, 0 };      /* Running flags 0=stopped, 1=starting, 2=pulsing */
uint8_t  uVoltages[CHANNELCOUNT][2] = {            /* Voltage setting pos/neg (default 2.5V up, 1.0V down) */
   { 25, 10 },
   { 25, 10 }
};
uint16_t uTimes[CHANNELCOUNT][TIMECOUNT] = {               /* Timing: start-pause, pos.pulse T1, interphase T2, neg.pule T3, period T4 */
   { 50000, 100, 10, 200, 10000 },
   { 50000, 100, 10, 200, 12000 }
};
uint16_t uDelta[CHANNELCOUNT][3] = {              /* Decrease delta (frequency increase) */
   { 100, 50000, 4 },
   { 0, 50000, 4 }
};
uint16_t pulseCount[CHANNELCOUNT] = { 0, 0 };      /* maximum pulses */

/***------------------------ Local functions ----------------------------***/

/***------------------------ Global functions ---------------------------***/
/*----------------------------------------------------------------------
    vInitWaveform
      Initialize this module

----------------------------------------------------------------------*/
void vInitWaveform( void )
{
   uint8_t  cnt;

   for ( cnt = 0; cnt < 2; cnt++ )
   {
      currentCount[cnt] = 0;
      currentState[cnt] = 0;
   }
}

/*--------------------------------------------------
 Generate waveforms within the RoundRobin system
 --------------------------------------------------*/
void vDoWaveform( void )
{
   currentCount[0] += 1;
}


/* EOF */
