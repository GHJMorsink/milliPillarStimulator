/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink

   Purpose:


   Contains:

   Module:
      Stimulator

------------------------------------------------------------------------------
*/


#ifndef BOARD_H_
#define BOARD_H_

#include <stdint.h>


#define POSITIVE     1
#define NEGATIVE     0


extern void vInitBoard(void);           /* Initialize all board items */

/*--------------------------------------------------
 The controls for the pulses
 --------------------------------------------------*/
extern void setVoltage(uint8_t channel, uint8_t decivolts);
extern void setHBridge(uint8_t channel, uint8_t side);
extern void clearHBridge(uint8_t channel);

#endif /* BOARD_H_ */

