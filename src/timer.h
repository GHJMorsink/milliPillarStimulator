/*----------------------------------------------------------------------------

 Copyright 2015, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements general timer functions

   Contains:

   Module:
------------------------------------------------------------------------------
*/

#ifndef TIMER_H_
#define TIMER_H_

/***------------------------- Defines ------------------------------------***/

#define SIXTEEN_MS  187
#define TEN_MS      117
#define FIVE_MS     58
#define TWO_MS      30

/***------------------------- Includes ----------------------------------***/
#include <stdint.h>

/***------------------------- Types -------------------------------------***/


/***------------------------ Global Data --------------------------------***/

/*  The value increments every 2/5/10ms */
extern uint16_t     uSystemTimerCounter;              /* counting */

/***------------------------ Global functions ---------------------------***/

/*--------------------------------------------------
 Initialize hardware
 --------------------------------------------------*/
extern void vInitTimer( uint8_t timing );

/*--------------------------------------------------
 Deliver a mutexed copy of the system timer
 --------------------------------------------------*/
extern void vGetSystemTimer( uint16_t *puTimer );

extern void delay_100us( uint16_t count);

#endif /* TIMER_H_ */
