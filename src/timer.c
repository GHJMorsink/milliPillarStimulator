/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink

   Purpose:
      Implements general timer functions

   Contains:

   Module:

------------------------------------------------------------------------------
*/

/***------------------------- Defines ------------------------------------***/

/***------------------------- Includes ----------------------------------***/

#include <avr/io.h>
#include <avr/interrupt.h>

#include "timer.h"

/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

uint16_t    uSystemTimerCounter;
uint8_t     uCurrentTimeConstant;

/***------------------------ Global Data --------------------------------***/

/***------------------------ Global functions ---------------------------***/
/*--------------------------------------------------
 Initialize hardware (interrupts are disabled)
 --------------------------------------------------*/
void vInitTimer( uint8_t timing )
{
   TCCR0  = 0x05;                       /* Prescaler 1024; CTC mode 0.0853 ms per count */
   TCNT0  = 256-timing;                 /* count starting at -58: every 5ms timer-overflow 117: 10ms*/
   TIMSK |= 0x01;                       /* At overflow enable interrupt */

   uCurrentTimeConstant = timing;
   uSystemTimerCounter = 0;
}


/***------------------------ Interrupt functions ------------------------***/
/*--------------------------------------------------
 System clock
 Runs at 5/10/16 ms per tick (32 ticks per second)
 --------------------------------------------------*/
#ifdef _lint
void TIMER0_OVF_vect( void )
#else
ISR(TIMER0_OVF_vect)
#endif
{
   TCNT0  = 256-uCurrentTimeConstant;   /* count starting at -58: every 5ms timer-overflow */
   uSystemTimerCounter += 1;
}

/*--------------------------------------------------
 Deliver a mutexed copy of the system timer
 --------------------------------------------------*/
void vGetSystemTimer( uint16_t *puTimer )
{
   cli();
   *puTimer = uSystemTimerCounter << 1; /* multiply by 2 for 'TWO_MS' */
   sei();
}

/* ToDo : delay() */


/* EOF */
