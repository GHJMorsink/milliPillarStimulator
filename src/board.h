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


#define POSITIVE        1
#define NEGATIVE        0

#define LED_PIN         5               /* pin SCL on ArduinoUNO */
#define LED_ON()        PORTC &= ~(1 << LED_PIN)
#define LED_OFF()       PORTC |= (1 << LED_PIN)


extern void vInitBoard(void);           /* Initialize all board items */

/*--------------------------------------------------
 The controls for the pulses
 --------------------------------------------------*/
extern void setVoltage(uint8_t channel, uint8_t decivolts);
extern void setHBridge(uint8_t channel, uint8_t side);
extern void clearHBridge(uint8_t channel);

#endif /* BOARD_H_ */

