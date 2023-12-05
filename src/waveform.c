/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink


   Purpose:
      Implements waveform generation

   Contains:

   Module:

------------------------------------------------------------------------------
*/
#include <stdint.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "waveform.h"
#include "timer.h"
#include "log.h"
#include "board.h"
#include "serial.h"

/***------------------------- Defines -----------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/
/* The data is given as flat types; structures give overhead in the generated code */
static uint8_t    currentState[CHANNELCOUNT];  /* FSM state */
static uint16_t   currentCount[CHANNELCOUNT];  /* count of pulses */
static uint16_t   currentTime[CHANNELCOUNT];   /* current time reference */
static uint16_t   currentPeriod[CHANNELCOUNT];   /* current period (T4) reference */
static uint16_t   currentCountPeriod[CHANNELCOUNT];   /* current pulses in this frequency period */
static uint8_t    uChangedPeriods[CHANNELCOUNT];  /* total changes */
/***------------------------ Global Data --------------------------------***/

//! Keep these in sequence and together, as they are stored in eeprom
sSetting_t   sSetChannel[CHANNELCOUNT];

uint8_t EEMEM  NonVolatileSettings[SETTING_SIZE];  /* eeprom copy */

/***------------------------ Local functions ----------------------------***/

static void vUpdateCurrentTime(uint8_t channel)
{
   if ( (sSetChannel[channel].pulseCount != 0) &&
        (currentCount[channel] >= sSetChannel[channel].pulseCount) )
   {
      vLogString(PSTR("FINISH"));
      print_uint16_base10( channel + 1 );
      vSendCR();
      sSetChannel[channel].uStartFlag = 0;
      return ;
   }
   if ( sSetChannel[channel].uDelta[0] == 0 )
   {
      currentPeriod[channel] = sSetChannel[channel].uTimes[4]; /* keep reference (could have been changed by the terminal) */
      return;                                                  /* This is a constant timing */
   }
   if ( currentCountPeriod[channel] >= sSetChannel[channel].uDelta[1])
   {
      /*--------------------------------------------------
        This period has to be changed
       --------------------------------------------------*/
      if ( currentPeriod[channel] > sSetChannel[channel].uDelta[0])  /* don't go into negative times */
      {
         currentPeriod[channel] -= sSetChannel[channel].uDelta[0];
      }
      uChangedPeriods[channel] += 1;
      if ( uChangedPeriods[channel] > sSetChannel[channel].uDelta[2])  /* check maximum changes */
      {
         uChangedPeriods[channel] = 0;
         currentPeriod[channel] = sSetChannel[channel].uTimes[4];
      }
      vLogString(PSTR("NewPeriod"));
      print_uint16_base10( channel + 1 );
      SendCommaSpace();
      print_uint16_base10( currentPeriod[channel] );
      vSendCR();

      currentCountPeriod[channel] = 0;
   }
}

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
      currentCountPeriod[cnt] = 0;
      uChangedPeriods[cnt] = 0;
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
      if ( (sSetChannel[i].uStartFlag == 0) || (sSetChannel[i].uStartFlag > 3) )  /* if set to 'off': set the FSM to startpoint */
      {
         currentState[i] = 0;
         currentCount[i] = 0;
         currentCountPeriod[i] = 0;
         uChangedPeriods[i] = 0;
      }
      vGetSystemTimer(&temp);              /* get the time */

      switch ( currentState[i] )           /* run for each channel the FSM */
      {
         case 0 :                          /* nothing happening, all in zero position */
            if ( sSetChannel[i].uStartFlag == 1 )
            {
               vLogString(PSTR("START"));
               print_uint16_base10( i + 1 );
               vSendCR();
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
               // vDebugHex(PSTR("\r\ntime1 "),(uint8_t *) &currentTime[i], 2);
               currentState[i] = 2;                   /* going to pulse gen */
            }
            break;
         case 2 :                          /* uninterupted pos.pulse,interphase,and neg.pulse */
            LED_ON();
            vSerialPutChar( 'A'+i );   /* show pulse on channel */
            // vDebugHex(PSTR("\r\nPulse "),(uint8_t *) &temp, 2);
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
            currentState[i] = 3;
            currentCount[i] += 1;               /* one pulse completed */
            currentCountPeriod[i] += 1;
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
               vUpdateCurrentTime(i);                 /* change -if applicable- the period time, */
                                                      /* and check max pulses */
               currentState[i] = 2;                   /* going to pulse gen */
               vGetSystemTimer(&currentTime[i]);      /* save current timecount */
               // vDebugHex(PSTR("\r\ntime3 "),(uint8_t *) &currentTime[i], 2);
            }
            break;
         default:                                     /* shouldn't occur */
            currentState[i] = 0;
            break;
      }
   }
}


/* EOF */
