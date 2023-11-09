/*----------------------------------------------------------------------------

 Copyright 2012, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements terminal functions

   Contains:

   Module:
      DOLF

This is the terminal to debug MMC, memory and related through the UART connection
------------------------------------------------------------------------------
*/
/*lint -e413 */

/***------------------------- Includes ----------------------------------***/
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <ctype.h>
#include <avr/wdt.h>
#include "terminal.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "board.h"
#include "log.h"
#include "serial.h"



/***------------------------- Defines ------------------------------------***/

#define MAXINPUTLENGTH     64           /* maximal command is 64 characters */
#define ACCESSCODE         0xC0DE       /* access to write to memory */

#define BS     0x08
#define BELL   0x07
#define CR     0x0D

/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

static char    acUserInput[MAXINPUTLENGTH];              /* Console input buffer */
static uint8_t uHalf = 0;

/***------------------------ Global Data --------------------------------***/
/*--------------------------------------------------

 --------------------------------------------------*/

/***------------------------- Types -------------------------------------***/

typedef void (USER_COMMAND)( char *argv );

/*--------------------------Prototypes for table---------------------------*/

static void  f_sm( char *argv );
static void  f_he( char *argv );
static void  f_wm( char *argv );
static void  f_ve( char *argv );
static void  f_st( char *argv );
static void  f_reboot( char *argv );
static void  f_ha( char *argv );

/***----------------------- Local Types ---------------------------------***/
static const struct sAccess
{
    USER_COMMAND    *pFunctionPointer;
    char            *szHelpText;
} asAccessArr[] = {
    { f_sm,     "SM  <0..FFFFFF>,<0..FF> Show Mem" },
    { f_he,     "HE  HElp" },
    { f_ve,     "VE  Show VErsion" },
    { f_reboot, "BO  <code> BOot" },
    { f_wm,     "WM  <code>,<0..FFFFFF>,<0..FF> Write Mem" },
    { f_st,     "ST  Show state" },
    { f_ha,     "HA  Only echo" }
};

static const int iAccArrSize = ( sizeof( asAccessArr ) / sizeof( struct sAccess ) );


/***------------------------ Local functions ----------------------------***/
#define vSendCharDiag   vSerialPutChar       /* the routine to output 1 character */
/*--------------------------------------------------
Write string
 --------------------------------------------------*/
static void vWriteString( char * const szString, unsigned char uLength )
{
   register unsigned char   uSentCount;

   for ( uSentCount = 0; uSentCount < uLength; uSentCount++ )
   {
      vSendCharDiag( (unsigned char) szString[ uSentCount ] );  /* send 1 character */
   }
}

/*----------------------------------------------------------------------
    vByteToHex
    Set string of two hex-characters from a byte
----------------------------------------------------------------------*/
static unsigned char cHex( unsigned char cRef )  /* use code */
{
   register unsigned char uRet;

   uRet = cRef + '0';
   if ( uRet > '9' )
   {
      uRet += 7;
   }
   return uRet;
}

static void vByteToHex( unsigned char cByte, char *szOutString )
{
    register unsigned char  cData;

    cData = cByte >> 4;
    szOutString[0] = (char) cHex(cData);
    cData = cByte & 0x0F;
    szOutString[1] = (char) cHex(cData);
    szOutString[2] = '\0';
}

/*--------------------------------------------------
vSendHByte
 --------------------------------------------------*/
static void vSendHByte( uint8_t uData )
{
    unsigned char  acSendMsg[3];

    vByteToHex( uData, (char *) acSendMsg );         /* convert it into a string */
    vSendCharDiag( acSendMsg[0] );          /* and send it */
    vSendCharDiag( acSendMsg[1] );
}

/*--------------------------------------------------
vSendLong
    show 32bit value including 2 spaces
 --------------------------------------------------*/
static void vSendLong( unsigned long lValue )
{
    vSendHByte( (uint8_t) (lValue >> 24) );   /* high byte */
    vSendHByte( (uint8_t) (lValue >> 16) );   /* med byte */
    vSendHByte( (uint8_t) (lValue >> 8) );    /* med byte */
    vSendHByte( (uint8_t) (lValue) );         /* low byte */
    vSendCharDiag( 0x20 );                  /* space after a word */
    vSendCharDiag( 0x20 );                  /* space after a word */
}

/*--------------------------------------------------
vSendCR
    send a new line
 --------------------------------------------------*/
static void vSendCR( void )
{
    vSendCharDiag( '\r' );
    vSendCharDiag( '\n' );
}

/*--------------------------------------------------
vShowPrompt
    Show the prompt to the user
 --------------------------------------------------*/
static void vShowPrompt( void )
{
    uint8_t i;

    vWriteString( "TERM> ", 6 );  /* show prompt */
    for ( i = 0; i < MAXINPUTLENGTH; i++ )
    {
       acUserInput[i] = '\0';      /* clear the inputline */
    }
}

/*--------------------------------------------------
Parameter error
 --------------------------------------------------*/
static void vShowParmError( void )
{
   vLogInfo( PSTR( "Parameter error" ));
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
iGetAddress
    Get an address from the command line
 --------------------------------------------------*/
static void vGetAddress( char *szAddress, unsigned long *iResult, uint8_t *uIndex )
{
   *iResult = 0;                   /* start with 0 */
   while( isxdigit( szAddress[ *uIndex ] ) )
   {
      if( isdigit( szAddress[ *uIndex ] ) )
      {
            *iResult = (*iResult << 4) + (unsigned long)((int)szAddress[ *uIndex ] - '0');
      }
      else
      {
            *iResult = (*iResult << 4) + (unsigned long)(((int)szAddress[ *uIndex ] - 'A') + 10);
      }
      *uIndex += 1;
   }
}

/*--------------------------------------------------
vShowMemory
    show a part of the memory starting at address 'start',
    with length 'size'
 --------------------------------------------------*/
static void vShowMemory( unsigned long iStart, unsigned int iSize )
{
   unsigned int  uLocation;
   uint8_t       *pByte;

   for ( uLocation = 0; uLocation < iSize; uLocation++ )
   {
      if ( (uLocation % 16) == 0 )    /* check we are on a boundary of 16 bytes */
      {
         vSendCR();                  /* new line */
         vSendLong( iStart + uLocation );  /* send value of address */
      }
      pByte = (uint8_t *) NULL;
      pByte += iStart + uLocation;
      vSendHByte( *pByte );           /* send it */
      vSendCharDiag( 0x20 );          /* space after byte */
   }
   vSendCR();                          /* new line */
}

/*--------------------------------------------------
vSetMemory
    Set a single byte in the memory to a value
 --------------------------------------------------*/
static void vSetMemory( unsigned long iStart, unsigned char iValue )
{
   uint8_t  *pByte;

   pByte = (uint8_t *) NULL;
   pByte += iStart;
   *(uint8_t *)pByte = iValue;
   vWriteString( "Memory set at: ", 15 );
   vSendLong( iStart );                /* send address */
   vSendHByte( *pByte );               /* send value */
   vSendCR();                          /* new line */
}

/*--------------------------------------------------
Commands
  Help
 --------------------------------------------------*/
static void f_he( char *argv )
{
   int iCount;

   (void) argv;
   vLogInfo( PSTR("HELP: First two characters are the command; implemented:") );
   vSendCR();
   for ( iCount = 0; iCount < iAccArrSize; iCount++ )
   {                                   /* write all strings */
       vWriteString( asAccessArr[ iCount ].szHelpText,
                     strlen( asAccessArr[ iCount ].szHelpText ) );
       vSendCR();
   }
}

/*--------------------------------------------------
Commands
  Reboot
 --------------------------------------------------*/
static void f_reboot( char *argv )
{
   uint8_t         uPoint;                 /* pointer into the argument string */
   unsigned long   iMemAddress;

   uPoint = 0;
   vGetAddress( argv, &iMemAddress, &uPoint );
   if ( iMemAddress == ACCESSCODE )        /* check correct access code is given */
   {
      vLogInfo( PSTR("Reboot") );
      wdt_enable(WDTO_15MS);
      for(;;)
      {
         ;                           /* wait on watchdog (16ms) */
      }
   }
   else
   {
      vShowParmError();
   }
}

/*--------------------------------------------------
Commands
  switch half mode
 --------------------------------------------------*/
static void f_ha( char *argv )
{
   (void) argv;
   uHalf = 1;
}


/*--------------------------------------------------
Commands
  Show memory
 --------------------------------------------------*/
static void f_sm( char *argv )
{
   unsigned long   iMemAddress;
   unsigned long   iSize;
   uint8_t         uPoint;                     /* pointer into the argument string */

   uPoint = 0;
   vGetAddress( argv, &iMemAddress, &uPoint ); /* get starting address */
   uPoint++;
   vGetAddress( argv, &iSize, &uPoint );        /* get size */
   vShowMemory( iMemAddress, iSize );           /* show bytes, with address */
}


/*--------------------------------------------------
Commands
  show state
 --------------------------------------------------*/
static void f_st( char *argv )
{
   uint8_t  uCState[4];

   (void) argv;
   vDebugHex( PSTR("Current status is"), uCState, 4 );
   vSendCR();
}

/*--------------------------------------------------
Commands
  Write memory
 --------------------------------------------------*/
static void f_wm( char *argv )
{
    unsigned long   iMemAddress;
    unsigned long   iValue;
    uint8_t         uPoint;             /* pointer into the argument string */

    uPoint = 0;
    vGetAddress( argv, &iMemAddress, &uPoint );       /* get access code */
    if ( iMemAddress == ACCESSCODE )                  /* check correct access code is given */
    {
        uPoint++;
        vGetAddress( argv, &iMemAddress, &uPoint );   /* get address */
        uPoint++;
        vGetAddress( argv, &iValue, &uPoint );        /* get value */
        vSetMemory( iMemAddress, (uint8_t) iValue );      /* set 1 byte, with address */
    }
    else
    {
        vShowParmError();
    }
}

/*--------------------------------------------------
Commands
  Show version
 --------------------------------------------------*/
static void f_ve( char *argv )
{
   (void) argv;
   vLogInfo( PSTR( "FULLVERSION" ));
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
   int     iCount;

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
         asAccessArr[iCount].pFunctionPointer( pszArgv[1] );
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
#define uReadChar    uSerialGetChar
static bool iCheckInputData( void )
{
   uint8_t    iCharacter;
   bool       iRc = false;                    /* no msg available */
   uint8_t    iCurrentPos;

   if ( uReadChar( &iCharacter ) == RESULT_SUCCESS )  /* is there a character? */
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
               vSendCharDiag( iCharacter );  /* echo BS */
               vSendCharDiag( 0x20 );        /* echo space */
               vSendCharDiag( iCharacter );  /* echo BS */
               acUserInput[ iCurrentPos - 1 ] = '\0';
            }
            else
            {
               vSendCharDiag( BELL );  /* beep */
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
               vSendCharDiag( BELL );  /* beep */
            }
            else
            {
               vSendCharDiag( iCharacter );  /* echo it */
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
   vLogInfo( PSTR("Terminal monitor for AVR"));
   f_ve( NULL );
   vShowPrompt();
}

/*--------------------------------------------------
 General looping for terminal
 --------------------------------------------------*/
void vDoTerminal( void )
{
   uint8_t    iCurrentPos;

   if ( iCheckInputData() )            /* read a command-line */
   {
      if ( uHalf == 0 )
      {
         vParseCommand();                /* do the command */
      }
      else
      {
         iCurrentPos = 0;
         while ( (iCurrentPos < MAXINPUTLENGTH) && (acUserInput[iCurrentPos] != '\0') )
         {
            iCurrentPos++;
         }
         vWriteString( acUserInput, iCurrentPos );
         vSendCR();
         if ( acUserInput[0] == 'H')
         {
            uHalf = 0;
         }
      }
      vShowPrompt();                  /* show prompt */
   }
}


/* EOF */
