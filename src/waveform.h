/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink


   Purpose:
      Implements waveform

   Contains:

   Module:

------------------------------------------------------------------------------
*/
#ifndef WAVE_H_
#define WAVE_H_

#define CHANNELCOUNT       2            /* ho many channels */
#define TIMECOUNT          5            /* all timing elements */

#include <stdint.h>

/***------------------------ Global Data --------------------------------***/

extern uint8_t  uStartFlag[CHANNELCOUNT];          /* Running flags 0=stopped, 1=starting, 2=pulsing */
extern uint8_t  uVoltages[CHANNELCOUNT][2];        /* Voltage setting pos/neg (default 2.5V up, 1.0V down) */
extern uint16_t uTimes[CHANNELCOUNT][TIMECOUNT];   /* Timing: start-pause, pos.pulse T1, interphase T2, neg.pule T3, period T4 */
extern uint16_t uDelta[CHANNELCOUNT][3];           /* Decrease delta (frequency increase) */
extern uint16_t pulseCount[CHANNELCOUNT];          /* maximum pulses */

/***------------------------ Global functions ---------------------------***/
/*----------------------------------------------------------------------
    vInitWaveform
      Initialize this module

----------------------------------------------------------------------*/
extern void vInitWaveform( void );

/*--------------------------------------------------
 Generate waveforms within the RoundRobin system
 --------------------------------------------------*/
extern void vDoWaveform( void );


#endif /* WAVE_H_ */

