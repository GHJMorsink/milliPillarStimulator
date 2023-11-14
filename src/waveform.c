/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink


   Purpose:
      Implements waveform generation

   Contains:

   Module:

------------------------------------------------------------------------------
*/
#include <stdint.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "waveform.h"
#include "timer.h"

/***------------------------- Defines -----------------------------------***/

#define LED_PIN         1
#define LED_ON()        PORTB &= ~(1 << LED_PIN)
#define LED_OFF()       PORTB |= (1 << LED_PIN)

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/
/* The data is given as flat types; structures give overhead in the generated code */
static uint8_t    currentState[CHANNELCOUNT];
static uint16_t   currentCount[CHANNELCOUNT];

/***------------------------ Global Data --------------------------------***/

//! Keep these in sequence and together, as they are stored in eeprom
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

#define SETTING_SIZE     ((CHANNELCOUNT) +                \
                          (CHANNELCOUNT * 2) +            \
                          (CHANNELCOUNT * TIMECOUNT * 2) +\
                          (CHANNELCOUNT * 3 * 2) +        \
                          (CHANNELCOUNT * 2))      /* size in bytes for all settings */

uint8_t EEMEM  NonVolatileSettings[SETTING_SIZE];

/***------------------------ Local functions ----------------------------***/

/***------------------------ Global functions ---------------------------***/
/*----------------------------------------------------------------------
    vInitWaveform
      Initialize this module

----------------------------------------------------------------------*/
void vInitWaveform( void )
{
   uint8_t  cnt;
   uint8_t  size;
   uint8_t  *ptr = &uStartFlag[0];      /* initialize at beginning of settings */

   for ( cnt = 0; cnt < CHANNELCOUNT; cnt++ )
   {
      currentCount[cnt] = 0;
      currentState[cnt] = 0;
   }
   for ( size = 0; size < SETTING_SIZE; size++ )  /* read all settings from eeprom */
   {
      *ptr = eeprom_read_byte(&NonVolatileSettings[size]);
      ptr++;
   }
}

/*--------------------------------------------------
 Generate waveforms within the RoundRobin system
 --------------------------------------------------*/
void vDoWaveform( void )
{
   uint8_t     i;
   uint16_t    temp;

   for ( i = 0; i < CHANNELCOUNT; i++)
   {
      if ( uStartFlag[i] == 0 )
      {
         currentState[i] = 0;
      }
   }
   switch ( currentState[i] )
   {
      case 0 :                          /* nothing happening, all in zero position */
         currentCount[i] = 0;
         if ( uStartFlag[i] != 0 )
         {
            currentState[i] = 1;        /* go to pre wait for starting pulsing */
            vGetSystemTimer(&currentCount[i]);  /* save current timecount */
         }
         break;
      case 1 :                          /* pre pulsing wait time */
         vGetSystemTimer(&temp);
         if ( temp > currentCount[i] )  /* time isn't rolled-over */
         {
            temp = temp - currentCount[i];
         } else
         {
            temp = temp  + (UINT16_MAX - currentCount[i]);  /* rolled-over! */
         }
         if ( temp >= uTimes[i][0])
         {
            currentState[i] = 2;        /* going to pulse gen */
            uStartFlag[i] = 2;          /* indicate it */
         }
         break;
      case 2 :                          /* uninterupted pos.pulse,interphase,and neg.pulse */
         LED_ON();
         //LED on
         /* set output voltage
            set positive outputs
            delay T1
            output off
            set output voltage
            delay T2
            set negative outputs
            delay T3
            output off
          */
         //LED off
         pulseCount[i] += 1;
         vGetSystemTimer(&currentCount[i]);
         currentState[i] = 3;
         LED_OFF();
         break;
      case 3 :                          /* Waiting for next pulse (in between the terminal can work) */
         vGetSystemTimer(&temp);
         if ( temp > currentCount[i] )  /* time isn't rolled-over */
         {
            temp = temp - currentCount[i];
         } else
         {
            temp = temp  + (UINT16_MAX - currentCount[i]);  /* rolled-over! */
         }
         if ( temp >= uTimes[i][4])     /* check period */
         {
            currentState[i] = 2;        /* going to pulse gen */
         }
         break;
      default:                          /* shouldn't occur */
         currentState[i] = 0;
         break;
   }
}


/* EOF */
