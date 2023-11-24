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
#include "log.h"
#include "board.h"

/***------------------------- Defines -----------------------------------***/

#define LED_PIN         1
#define LED_ON()        PORTB &= ~(1 << LED_PIN)
#define LED_OFF()       PORTB |= (1 << LED_PIN)

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/
/* The data is given as flat types; structures give overhead in the generated code */
static uint8_t    currentState[CHANNELCOUNT];  /* FSM state */
static uint16_t   currentCount[CHANNELCOUNT];  /* count of pulses */
static uint16_t   currentTime[CHANNELCOUNT];   /* current time reference */
static uint16_t   currentPeriod[CHANNELCOUNT];   /* current period (T4) reference */

/***------------------------ Global Data --------------------------------***/

//! Keep these in sequence and together, as they are stored in eeprom
sSetting_t   sSetChannel[CHANNELCOUNT];

uint8_t EEMEM  NonVolatileSettings[SETTING_SIZE];  /* eeprom copy */

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
   uint8_t  *ptr = &sSetChannel[0].uStartFlag;      /* initialize at beginning of settings */

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

   vGetSystemTimer(&temp);
   for ( i = 0; i < CHANNELCOUNT; i++)
   {
      if ( (sSetChannel[i].uStartFlag == 0) || (sSetChannel[i].uStartFlag > 3) )  /* if set to 'off': set the FSM to startpoint */
      {
         currentState[i] = 0;
         currentCount[i] = 0;
      }

      switch ( currentState[i] )           /* run for each channel the FSM */
      {
         case 0 :                          /* nothing happening, all in zero position */
            if ( sSetChannel[i].uStartFlag == 1 )
            {
               vDebugHex(PSTR("START "),(uint8_t *) &i, 1);
               currentState[i] = 1;        /* go to pre wait for starting pulsing */
               currentTime[i] = temp;      /* set current time */
               currentPeriod[i] = sSetChannel[i].uTimes[4];  /* set period reference */
            } else
            {
               sSetChannel[i].uStartFlag = 0;
            }
            break;
         case 1 :                          /* pre pulsing wait time */
            if ( temp >= currentTime[i] )  /* time isn't rolled-over */
            {
               temp = temp - currentTime[i];
            } else
            {
               temp = temp  + (UINT16_MAX - currentTime[i]);  /* rolled-over! */
            }
            if ( temp >= sSetChannel[i].uTimes[0])
            {
               sSetChannel[i].uStartFlag = 2;         /* indicate it */
               vGetSystemTimer(&currentTime[i]);      /* save current timecount */
               vDebugHex(PSTR("\r\ntime1 "),(uint8_t *) &currentTime[i], 2);
               currentState[i] = 2;                   /* going to pulse gen */
            }
            break;
         case 2 :                          /* uninterupted pos.pulse,interphase,and neg.pulse */
            LED_ON();
            vDebugHex(PSTR("\r\nPulse "),(uint8_t *) &temp, 2);
            setVoltage(i, sSetChannel[i].uVoltages[0]);  /* set positive output voltage */
            setHBridge(i, POSITIVE);  /* start the pulse */
            delay_100us(sSetChannel[i].uTimes[1]);
            clearHBridge(i);         /* no output */
            if ( sSetChannel[i].uTimes[2] > 0)
            {
               delay_100us(sSetChannel[i].uTimes[2]);
            }
            setVoltage(i, sSetChannel[i].uVoltages[1]);  /* set negative output voltage */
            if ( sSetChannel[i].uTimes[3] > 0)
            {
               setHBridge(i, NEGATIVE);  /* start the pulse */
               delay_100us(sSetChannel[i].uTimes[3]);
               clearHBridge(i);         /* no output */
            }
            vGetSystemTimer(&temp);
            vDebugHex(PSTR("\r\nNoPulse "),(uint8_t *) &temp, 2);
            currentState[i] = 3;
            currentCount[i] += 1;               /* one pulse completed */
            LED_OFF();
            break;
         case 3 :                          /* Waiting for next pulse (in between the terminal can work) */
            if ( temp >= currentTime[i] )  /* time isn't rolled-over */
            {
               temp = temp - currentTime[i];
            } else
            {
               temp = temp  + (UINT16_MAX - currentTime[i]);  /* rolled-over! */
            }
            if ( temp >= sSetChannel[i].uTimes[4])     /* check period */
            {
               currentState[i] = 2;                   /* going to pulse gen */
               vGetSystemTimer(&currentTime[i]);      /* save current timecount */
               vDebugHex(PSTR("\r\ntime3 "),(uint8_t *) &currentTime[i], 2);
            }
            break;
         default:                                     /* shouldn't occur */
            currentState[i] = 0;
            break;
      }
   }
}


/* EOF */
