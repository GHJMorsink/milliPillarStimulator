/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink

   Purpose:
      Implements general timer functions

   Contains:

   Module:

------------------------------------------------------------------------------
*/

/***------------------------- Defines ------------------------------------***/

#define T1TIME_100US    19230   /* for 8MHz clock */

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
   TCCR0  = 5; //0x04;                  /* Prescaler div 256; CTC mode 0.0853/4 ms per count */
   TCNT0  = 256-timing;                 /* count starting at -58: every 5ms timer-overflow 117: 10ms*/
   TIMSK |= 0x01;                       /* At overflow enable interrupt */

   uCurrentTimeConstant = timing;
   uSystemTimerCounter = 0;

   /* timer1 is used as a 100us delay timer */
   /* setup: normal mode WGM13:0 = 0, TCNT1 determines time until TOV1 will be 1 (overflow) */
   TCCR1A = 0;
   TCCR1B = 4; /* clock is crystal/64 --> 1us on 8MHz system, 0.5us on 16Mhz system */
   TCNT1 = 65536 - T1TIME_100US;
   TIFR &= ~(1 << TOV1);   /* clear TOV1 */
   // TIFR =TIFR & 0xFB;

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

#pragma GCC push_options
#pragma GCC optimize ("O0")             /* no optimization! */
/* Delay function for 100 us:  waits count times 100us */
void delay_100us( uint16_t count)
{
   register uint16_t i;

   for (i = 0; i < count; i++)
   {
      asm ("");                           /* the loop sould not be optimized */
      TCNT1 = 65536 - T1TIME_100US;
      TIFR &= ~(1 << TOV1);               /* clear TOV1 */
      // TIFR = TIFR & 0xFB;                 /* clear TOV1 */
      loop_until_bit_is_set(TIFR, TOV1);  /* wait until the flag is set */
   }

}
#pragma GCC pop_options

/* EOF */
