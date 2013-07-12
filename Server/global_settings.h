//Overall settings
// Author: Vincent Gijsen

/**
*    Packet Format |PACKET_COUNTER/IDENT 1BYTE|NODES ADDRESS 2BYTE|PACKETTYPE 1BYTE|PAYLOAYD rest of bytes|
**/

#ifndef GLOBAL_SETTINGS_H_
#define GLOBAL_SETTINGS_H_

#if defined(__MSP430G2552__)
#define BIGCHIP
#endif

#define REG_PACK_COUNTER 0
#define REG_PACK_ADDRMSB 1
#define REG_PACK_ADDRLSB 2
#define REG_PACK_TYPESET 3
#define REG_PACK_VAL0 4
#define REG_PACK_VAL1 5
#define REG_PACK_VAL2 6


#define LISTEN_ADDRESS   {'M','U','L','T','I'}

#define C1 {'L','1'}
#define C2 {'L','2'}
#define C3 {'L','3'}
#define C4 {'L','4'}

#define R1 {'R','1'}
#define R2 {'R','2'}
#define R3 {'R','3'}
#define R4 {'R','4'}


//Radio Settings
#define RADIO_SPEED 1000000
#define RADIO_CHANNEL 125
#define CMD P2_0
#define CSN P2_1
#define IRQ P2_2


//Packet Types
#define PACKET_TEMP 'T'
#define PACKET_BATT 'B'
#define PACKET_BRIGHTNESS 'd'
#define PACKET_RGB 'r'
#define PACKET_SWITCH 's'
#define PACKET_PING 'p'

//Sampling setting (temperature)
#define SAMPLES 4


//Note, pin 1_0 and 2_3 cannot be used for pwm
#define LED_DRIVER_R P2_6
#define LED_DRIVER_G P2_5
#define LED_DRIVER_B P2_4

//Definitions for Sleep and fading times
#define BLINK_DELAY 10
#define BLINK_ERROR 1000
#define POWERNAP 2
#define TOPCOUNTER 5000
#define FADEUPDATE 10

#ifndef BIGCHIP
  #pragma message  ("Compiling for SMALLCHIP")
  #define LEDLENGTH 1
  #define LED_DRIVER_R P1_2
#else
  #pragma message  ("Compiling for BIGCHIP")
  #define LEDLENGTH 3
  #define LED_DRIVER_R P2_6
#endif


#define LED_DRIVER_G P2_5
#define LED_DRIVER_B P2_4


#endif /* SETTINGS */

#if defined(__MSP430G2452__) || defined(__MSP430G2553__) || defined(__MSP430G2231__) 
#pragma message  ("Board supported, CPU")
#else
#error Board not supported
#endif

