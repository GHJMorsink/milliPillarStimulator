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

#define CHANNELCOUNT       2            /* how many channels */
#define TIMECOUNT          5            /* all timing elements */

#include <stdint.h>

/***------------------------ Global Data --------------------------------***/
typedef struct sSetting
{
   uint8_t  uStartFlag;                /* Running flags 0=stopped, 1=starting, 2=pulsing */
   uint8_t  uVoltages[2];              /* Voltage setting pos/neg (default 2.5V up, 1.0V down) */
   uint16_t uTimes[TIMECOUNT];         /* Timing: start-pause, pos.pulse T1, interphase T2, neg.pule T3, period T4 */
   uint16_t uDelta[3];                 /* Decrease delta (frequency increase) */
   uint16_t pulseCount;                /* maximum pulses */
} sSetting_t;

extern sSetting_t   sSetChannel[CHANNELCOUNT];
#define  SETTING_SIZE   (sizeof(sSetChannel))

extern uint8_t  NonVolatileSettings[SETTING_SIZE];

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

