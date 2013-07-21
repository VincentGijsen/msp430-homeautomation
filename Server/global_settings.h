
//Overall settings
// Author: Vincent Gijsen

/**
*    Packet Format |PACKET_COUNTER/IDENT 1BYTE|NODES ADDRESS 2BYTE|PACKETTYPE 1BYTE|PAYLOAYD rest of bytes|
**/

#ifndef GLOBAL_SETTINGS_H_
#define GLOBAL_SETTINGS_H_

#if defined(__MSP430G2553__)
  #define BIGCHIP
#endif

//#define REG_PACK_COUNTER 0
//#define REG_PACK_ADDRMSB 1
//#define REG_PACK_ADDRLSB 2

#define REG_ADD1 0
#define REG_ADD2 1
#define REG_ADD3 2
#define REG_ADD4 3
#define REG_ADD5 4
#define REG_PACK_TYPESET 5
#define REG_PACK_VAL0 6
#define REG_PACK_VAL1 7
#define REG_PACK_VAL2 8
#define REG_PACK_VAL3 9


#define PC_PACKAGE_TYPE 5
#define PC_VAL0 6
#define PC_VAL1 7
#define PC_VAL2 8


#define SERVER_ADDRESS {'M','U','L','T','I'}


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
#define PACKET_ACK 'a'

//Sampling setting (temperature)
#define SAMPLES 4

//generation of minor id + delay
#ifndef MINOR
#define MINOR 0x40
#endif


//Definitions for Sleep and fading times
#define BLINK_DELAY 5
#define BLINK_ERROR 1000
#define POWERNAP 2
#define TOPCOUNTER 0x0FFF
#define FADEUPDATE 1
#define MAXBRIGHNESS 200


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


#if defined(__MSP430G2452__) || defined(__MSP430G2553__) || defined(__MSP430G2231__) 
#pragma message  ("Board supported, CPU")
#else
#error Board not supported
#endif

#pragma message ("info:")
#pragma message "lsb address:" XSTR(LED_DRIVER_B)
#pragma message (ADRESSDELAY)


#endif /* SETTINGS */

