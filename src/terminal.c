/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink

   Purpose:
      Implements terminal functions

   Contains:

   Module:
      Stimulator

------------------------------------------------------------------------------
*/
/*lint -e413 */

/***------------------------- Includes ----------------------------------***/
#include <avr/io.h>
#ifdef _lint
 #ifdef ____ATTR_PURE__
   #undef __ATTR_PURE__
 #endif
 #ifdef __attribute__
   #undef __attribute__
 #endif
 #define __ATTR_PURE__
 #define __attribute__(var)
#endif
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

#include <string.h>
#include <ctype.h>

#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "log.h"
#include "serial.h"
#include "waveform.h"                   /* for accesss to the settings */
#include "terminal.h"



/***------------------------- Defines ------------------------------------***/

#define MAXINPUTLENGTH     64           /* maximal command is 64 characters */
#define MAXSENDLENGTH      80           /* maximal sending length for strings; before waits needed */
#define MAX_INT_DIGITS     5            /* maximal digits in a uint16 */

#define BS     0x08
#define BELL   0x07
#define CR     0x0D

/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

static char    acUserInput[MAXINPUTLENGTH];              /* Console input buffer */

/***------------------------ Global Data --------------------------------***/
/*--------------------------------------------------

 --------------------------------------------------*/

/***------------------------- Types -------------------------------------***/

typedef void (USER_COMMAND)( char *argv );

/*--------------------------Prototypes for table---------------------------*/

static void  f_he( char *argv );
static void  f_ve( char *argv );
static void  f_ru( char *argv );
static void  f_of( char *argv );
static void  f_ss( char *argv );
static void  f_sv( char *argv );
static void  f_st( char *argv );
static void  f_sd( char *argv );
static void  f_sc( char *argv );
static void  f_wr( char *argv );
static void  f_bo( char *argv );

/***----------------------- Local Types ---------------------------------***/
static const struct sAccess
{
    USER_COMMAND    *pFunctionPointer;
    char            *szHelpText;
} asAccessArr[] = {
    { f_he,     "HE  HElp" },
    { f_ve,     "VE  Show VErsion" },
    { f_ru,     "RU  <1..4> RUn Start pulses" },
    { f_of,     "OF  Set all outputs OFf (or <1..4>)" },
    { f_bo,     "BO  BOot/reset (firmware update)" },
    { f_ss,     "SS  Show Settings" },
    { f_sv,     "SV  <1..4>,<0..50>,<0..50> Set Voltage; pos. and neg. pulse" },
    { f_st,     "ST  <1..4>,<0..65535>,..,<0..65535> Set Timing; 5 timing parms" },
    { f_sd,     "SD  <1..4>,<0..65535>,<0..65535>,<0..255> Set Delta timing" },
    { f_sc,     "SC  <1..4>,<0..65535> Set repeat count" },
    { f_wr,     "WR  Write/store all settings" }
};

#define  iAccArrSize (sizeof(asAccessArr) / sizeof(struct sAccess))


/***------------------------ Local functions ----------------------------***/
/*--------------------------------------------------
 Extracts an integer floating pointvalue from a string
 --------------------------------------------------*/
uint8_t read_uint(char *line, uint8_t *char_counter, uint16_t *variable_ptr)
{
    char       *ptr = line + *char_counter;
    uint8_t    c;
    uint16_t   intval = 0;
    uint8_t    ndigit = 0;

    while(1)
    {
         /* Grab first character and increment pointer. No spaces assumed in line. */
        c = (uint8_t) *ptr++;
        c -= '0';
        if (c <= 9)
        {
            ndigit++;
            if (ndigit <= MAX_INT_DIGITS)
            {
                intval = (((intval << 2) + intval) << 1) + c; /* intval*10 + c */
            } // else  Drop overflow digits
        } else {
            break;
        }
    }

    // Return if no digits have been read.
    if (!ndigit) { return(false); };

    // prepare output.
    *variable_ptr = intval;
    *char_counter = (ptr - line) - 1; // Set char_counter to next statement
    return(true);
}

/*--------------------------------------------------
Wait for room in serial output buffer
 --------------------------------------------------*/
static void waitPrint(void)
{
   while ( uSerialGetFree() < MAXSENDLENGTH )
   {
      ;
   }
}

/*--------------------------------------------------
Write string
 --------------------------------------------------*/
static void vWriteString( char * const szString, unsigned char uLength )
{
   register unsigned char   uSentCount;

   waitPrint();                         /* wait for room to print */
   for ( uSentCount = 0; uSentCount < uLength; uSentCount++ )
   {
      vSerialPutChar( (unsigned char) szString[ uSentCount ] );  /* send 1 character */
   }
}

/*--------------------------------------------------
vShowPrompt
    Show the prompt to the user
 --------------------------------------------------*/
static void vShowPrompt( void )
{
    uint8_t i;

    vLogString( PSTR( "TERM>" ));   /* show prompt */
    for ( i = 0; i < MAXINPUTLENGTH; i++ )
    {
       acUserInput[i] = '\0';      /* clear the inputline */
    }
}

/*--------------------------------------------------
Parameter error
 --------------------------------------------------*/
static void vShowParmError( uint8_t type )
{
   if ( type == 0 )
   {
      vLogString( PSTR( "Parameter out of bounds" ) );
   } else
   {
      vLogString( PSTR( "Parameter missing" ) );
   }
   vLogInfo( PSTR( "error" ) );
}

/*--------------------------------------------------
isspace
    simple check on space-characters
 --------------------------------------------------*/
static bool fIsSpace( char cInput )
{
   switch ( cInput ) {
      case ',' :
      case ':' :
      case ' ' :
      case '\n':
      case '\t' :
      case '\0' :
         return true;
      default  :
         return false;
   }
}

/*--------------------------------------------------
Commands
  Help
 --------------------------------------------------*/
static void f_he( char *argv )
{
   uint8_t iCount;

   (void) argv;
   vLogInfo( PSTR("HELP: First two characters are the command; implemented:") );
   vSendCR();
   for ( iCount = 0; iCount < iAccArrSize; iCount++ )
   {                                   /* write all strings */
       vWriteString( asAccessArr[ iCount ].szHelpText,
                     (uint8_t) strlen( asAccessArr[ iCount ].szHelpText ) );
       vSendCR();
   }
}

/*--------------------------------------------------
Commands
  Show version
 --------------------------------------------------*/
static void f_ve( char *argv )
{
   (void) argv;
   vLogInfo( PSTR( "Stimulator Version 2.00.00" ));
}

/*--------------------------------------------------
Commands
  Boot
 --------------------------------------------------*/
static void f_bo( char *argv )
{
   (void) argv;
   vLogInfo( PSTR("Reboot") );
   wdt_enable(WDTO_60MS);
   for(;;)
   {
      ;                           /* wait on watchdog (60ms) */
   }
}

/*--------------------------------------------------
Commands
  run
 --------------------------------------------------*/
static void f_ru( char *argv )
{
   uint16_t   iChannel;
   uint8_t    uPoint;             /* pointer into the argument string */
   uint8_t    iRc;

   uPoint = 0;
   iRc = read_uint( argv, &uPoint, &iChannel ); /* get channel to work on */
   if (! iRc)
   {
      for(uPoint = 0; uPoint < CHANNELCOUNT; uPoint++)
      {
         sSetChannel[uPoint].uStartFlag = 1;    /* start all */
      }
   }
   else if ( (iChannel == 0) || (iChannel > CHANNELCOUNT) )
   {
      vShowParmError(0);
   }
   else
   {
      sSetChannel[iChannel - 1].uStartFlag = 1;    /* start specific */
   }
}

/*--------------------------------------------------
Commands
  Off, Stop all or specific
 --------------------------------------------------*/
static void f_of( char *argv )
{
   uint16_t   iChannel;
   uint8_t    uPoint;             /* pointer into the argument string */
   uint8_t    iRc;

   uPoint = 0;
   iRc = read_uint( argv, &uPoint, &iChannel ); /* get channel to work on */
   if (! iRc)
   {
      vLogInfo( PSTR( "Off" ));
      for(uPoint = 0; uPoint < CHANNELCOUNT; uPoint++)
      {
         sSetChannel[uPoint].uStartFlag = 0;    /* stop all */
      }
   }
   else if ( (iChannel == 0) || (iChannel > CHANNELCOUNT) )
   {
      vShowParmError(0);
   }
   else
   {
      sSetChannel[iChannel - 1].uStartFlag = 0;    /* stop specific */
   }
}

/*--------------------------------------------------
Commands
  Show settings
 --------------------------------------------------*/
static void f_ss( char *argv )
{
   uint8_t  i, j;

   (void) argv;
   vLogInfo( PSTR( "Settings:" ));
   for ( i = 0; i < CHANNELCOUNT; i++)
   {
      vSendCR();
      waitPrint();                         /* wait for room to print */
      vLogString( PSTR( "Settings channel:      " ));
      print_uint16_base10(i+1);
      vSendCR();
      waitPrint();                         /* wait for room to print */
      vLogString( PSTR( "Voltage V1, V2:        " ));
      print_uint16_base10(sSetChannel[i].uVoltages[0]);
      SendCommaSpace();
      print_uint16_base10(sSetChannel[i].uVoltages[1]);
      vSendCR();
      waitPrint();                         /* wait for room to print */
      vLogString( PSTR( "Timings T0,T1,T2,T3,T4:" ));
      for ( j = 0; j < 5; j++)
      {
         print_uint16_base10(sSetChannel[i].uTimes[j]);
         if ( j < 4 )
         {
            SendCommaSpace();
         }
      }
      vSendCR();
      waitPrint();                         /* wait for room to print */
      vLogString( PSTR( "Delta DT, DP, DM:      " ) );
      for ( j = 0; j < 3; j++)
      {
         print_uint16_base10(sSetChannel[i].uDelta[j]);
         if ( j < 2 )
         {
            SendCommaSpace();
         }
      }
      vSendCR();
      waitPrint();                         /* wait for room to print */
      vLogString( PSTR( "Pulse Counts RPT:      " ));
      print_uint16_base10(sSetChannel[i].pulseCount);
      vSendCR();
   }
}

/*--------------------------------------------------
Commands
  Set voltages
 --------------------------------------------------*/
static void f_sv( char *argv )
{
   uint16_t   iChannel;
   uint16_t   uVolts[2];
   uint8_t    uPoint;                                 /* pointer into the argument string */
   uint8_t    iRc;

   uPoint = 0;
   iRc = read_uint( argv, &uPoint, &iChannel );       /* get channel to work on */
   if (! iRc)
   {
      vShowParmError(1);
      return;
   }
   if ( (iChannel == 0) || (iChannel > CHANNELCOUNT) )
   {
      vShowParmError(0);
      return;
   }
   uPoint++;
   iRc = read_uint( argv, &uPoint, &uVolts[0] );      /* get V1 */
   if ( iRc )
   {
      uPoint++;
      iRc = read_uint( argv, &uPoint, &uVolts[1] );   /* get V2 */
   }
   if (! iRc)
   {
      vShowParmError(1);
      return;
   }
   if ( (uVolts[0] > 50) || (uVolts[1] > 50) )
   {
      vShowParmError(0);
      return;
   }
   /* Set the resulting parameters */
   iChannel -= 1;
   sSetChannel[iChannel].uVoltages[0] = (uint8_t) uVolts[0];
   sSetChannel[iChannel].uVoltages[1] = (uint8_t) uVolts[1];
}

/*--------------------------------------------------
Commands
  Set times
 --------------------------------------------------*/
static void f_st( char *argv )
{

   uint16_t   iChannel;
   uint16_t   uTempTimes[TIMECOUNT];
   uint8_t    uPoint;                           /* pointer into the argument string */
   uint8_t    iRc;
   uint8_t    i;

   uPoint = 0;
   iRc = read_uint( argv, &uPoint, &iChannel ); /* get channel to work on */
   if (! iRc)
   {
      vShowParmError(1);
      return;
   }
   if ( (iChannel == 0) || (iChannel > CHANNELCOUNT) )
   {
      vShowParmError(0);
      return;
   }
   uPoint++;
   for ( i = 0; i < TIMECOUNT; i++ )
   {
      iRc = read_uint( argv, &uPoint, &uTempTimes[i] ); /* get Tx */
      if (! iRc)
      {
         vShowParmError(1);
         return;
      }
      uPoint++;
   }
   /* Set the resulting parameters */
   iChannel -= 1;
   for ( i = 0; i < TIMECOUNT; i++ )
   {
      sSetChannel[iChannel].uTimes[i] = uTempTimes[i];
   }
}

/*--------------------------------------------------
Commands
  Set delta
 --------------------------------------------------*/
static void f_sd( char *argv )
{
   uint16_t   iChannel;
   uint16_t   uTempDelta[3];
   uint8_t    uPoint;             /* pointer into the argument string */
   uint8_t    iRc;
   uint8_t    i;

   uPoint = 0;
   iRc = read_uint( argv, &uPoint, &iChannel ); /* get channel to work on */
   if (! iRc)
   {
      vShowParmError(1);
      return;
   }
   if ( (iChannel == 0) || (iChannel > CHANNELCOUNT) )
   {
      vShowParmError(0);
      return;
   }
   uPoint++;
   for ( i = 0; i < 3; i++ )
   {
      iRc = read_uint( argv, &uPoint, &uTempDelta[i] ); /* get DT DP DM */
      if (! iRc)
      {
         vShowParmError(1);
         return;
      }
      uPoint++;
   }
   if ( uTempDelta[2] > 10 )
   {
      vShowParmError(0);
      return;
   }
   /* Set the resulting parameters */
   iChannel -= 1;
   for ( i = 0; i < 3; i++ )
   {
      sSetChannel[iChannel].uDelta[i] = uTempDelta[i];
   }
}

/*--------------------------------------------------
Commands
  Set count
 --------------------------------------------------*/
static void f_sc( char *argv )
{
   uint16_t   iChannel;
   uint16_t   uTempCount;
   uint8_t    uPoint;             /* pointer into the argument string */
   uint8_t    iRc;

   uPoint = 0;
   iRc = read_uint( argv, &uPoint, &iChannel ); /* get channel to work on */
   if (! iRc)
   {
      vShowParmError(1);
      return;
   }
   if ( (iChannel == 0) || (iChannel > CHANNELCOUNT) )
   {
      vShowParmError(0);
      return;
   }
   uPoint++;
   iRc = read_uint( argv, &uPoint, &uTempCount ); /* get RPT */
   if (! iRc)
   {
      vShowParmError(1);
      return;
   }
   /* Set the resulting parameters */
   sSetChannel[iChannel - 1].pulseCount = uTempCount;
}

/*--------------------------------------------------
Commands
  Store to eeprom
 --------------------------------------------------*/
static void f_wr( char *argv )
{
   uint8_t  size;
   uint8_t  *ptr = &sSetChannel[0].uStartFlag;      /* beginning of settings
                                          */
   (void) argv;
   vLogInfo( PSTR( "Writing to eeprom" ));
   for ( size = 0; size < SETTING_SIZE; size++ )
   {
      eeprom_update_byte( &NonVolatileSettings[size], *ptr );
      ptr++;
   }
}

/*--------------------------------------------------
fCompareTwo
    compare first two characters from two strings
--------------------------------------------------*/
static bool fCompareTwo( char *szFirst, char *szSecond )
{
    if ( (szFirst[0] == szSecond[0]) &&
         (szFirst[1] == szSecond[1]) )
    {
        return true;
    }
    return false;
}

/*--------------------------------------------------
vParseCommand
    check whats in the command buffer and react on it
    The buffer is filled
 --------------------------------------------------*/
static void vParseCommand( void )
{
   char    *pcCurrent;                 /* current character */
   char    *pszArgv[2];
   uint8_t iCount;

   pcCurrent = acUserInput;            /* the buffer from the serial line */

   while( (*pcCurrent != '\0') &&  (fIsSpace( *pcCurrent ) == true ) )
   {
      pcCurrent++;                    /* remove leading spaces */
   }
   pszArgv[0] = pcCurrent;             /* mnemonic or empty string   */

   while( (*pcCurrent != '\0') &&  (fIsSpace( *pcCurrent ) == false ) )
   {
      pcCurrent++;                    /* skip token */
   }
   if ( *pcCurrent != '\0' )
   {
      *pcCurrent++ = '\0';           /* and terminate with '\0'    */
   }

   while( (*pcCurrent != '\0') &&  (fIsSpace( *pcCurrent ) == true ) )
   {
      pcCurrent++;                    /* remove leading spaces */
   }
   pszArgv[1] = pcCurrent;             /* argument(s) or empty string */
   iCount = 0;
   if ( strlen( pszArgv[0] ) > 1 )     /* minimal 2 characters */
   {
      while ( ( iCount < iAccArrSize ) &&
              ( (fCompareTwo( pszArgv[0], asAccessArr[ iCount].szHelpText ) == false ) ) )
      {
         iCount++;
      }
      if ( iCount < iAccArrSize )
      {
         /* Known command found == [iCount] */
         asAccessArr[iCount].pFunctionPointer( pszArgv[1] );  /* execute the request */
      }
      else
      {                               /* the command is not in the list */
         vLogInfo( PSTR( "Unknown command" ));
         f_he( NULL );
      }
   }
}

/*----------------------------------------------------------------------
Check the serial input on incoming data, if full request, return TRUE, else FALSE
----------------------------------------------------------------------*/
static bool iCheckInputData( void )
{
   uint8_t    iCharacter;
   bool       iRc = false;                    /* no msg available */
   uint8_t    iCurrentPos;

   if ( uSerialGetChar( &iCharacter ) == RESULT_SUCCESS )  /* is there a character? */
   {
      switch ( iCharacter )
      {
         case BS :
            iCurrentPos = 0;
            while ( (iCurrentPos < MAXINPUTLENGTH) && (acUserInput[iCurrentPos] != '\0') )
            {
               iCurrentPos++;
            }
            if ( iCurrentPos > 0 )
            {
               vSerialPutChar( iCharacter );  /* echo BS */
               vSerialPutChar( 0x20 );        /* echo space */
               vSerialPutChar( iCharacter );  /* echo BS */
               acUserInput[ iCurrentPos - 1 ] = '\0';
            }
            else
            {
               vSerialPutChar( BELL );  /* beep */
            }
            break;

         case CR :
            vSendCR();                  /* and ready */
            iRc = true;
            break;

         default:
            iCurrentPos = 0;
            while ( (iCurrentPos < MAXINPUTLENGTH) && (acUserInput[iCurrentPos] != '\0') )
            {
               iCurrentPos++;
            }
            if ( iCurrentPos == MAXINPUTLENGTH )
            {
               vSerialPutChar( BELL );  /* beep */
            }
            else
            {
               vSerialPutChar( iCharacter );  /* echo it */
               acUserInput[iCurrentPos] = (char) toupper( iCharacter );
            }
            break;
         }
   }
   return iRc;
}


/***------------------------ Global functions ---------------------------***/
/*--------------------------------------------------
 Initialisation with general text
 --------------------------------------------------*/
void vTerminalInit( void )
{
   vSendCR();
   vLogInfo( PSTR("Command Terminal for stimulator"));
   f_ve( NULL );
   vShowPrompt();
}

/*--------------------------------------------------
 General looping for terminal
 --------------------------------------------------*/
void vDoTerminal( void )
{
   if ( iCheckInputData() )            /* read a command-line */
   {
      vParseCommand();                /* do the command */
      vShowPrompt();                  /* show prompt */
   }
}


/* EOF */
