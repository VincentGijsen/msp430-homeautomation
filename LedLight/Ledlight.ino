//Define if simple Dimmer, else RGB

//#define SMALLCHIP true

//0x40 == @ +1 > A
//#define PRINT_TOKEN(token) printf(#token " is %d", token)

#define MINOR 0x40
#define INDEX 2

#include "global_settings.h"

#include <Enrf24.h>
#include <nRF24L01.h>
#include <string.h>
#include <SPI.h>
//#include <Utils.h>

//ENABLE SERIAL AND PRINT DEBUGGIN
#define VERBOSE 0

Enrf24 radio(CMD, CSN, IRQ);


const uint8_t myAddress[5] = {'L', 'E', 'D', 'R', '1'};
const uint8_t serverAddress[5] = SERVER_ADDRESS;

uint16_t loopCounter=0;

uint8_t setPoint[3];
uint8_t current[3];

const unsigned char linear_brightness_curve[256] = { 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   1,   1,   1,   1,   1,   1, 
    1,   1,   1,   1,   1,   1,   1,   1, 
    1,   1,   1,   1,   1,   1,   1,   1, 
    1,   1,   2,   2,   2,   2,   2,   2, 
    2,   2,   2,   2,   2,   2,   2,   2, 
    2,   3,   3,   3,   3,   3,   3,   3, 
    3,   3,   3,   3,   3,   4,   4,   4, 
    4,   4,   4,   4,   4,   4,   5,   5, 
    5,   5,   5,   5,   5,   5,   6,   6, 
    6,   6,   6,   6,   6,   7,   7,   7, 
    7,   7,   8,   8,   8,   8,   8,   9, 
    9,   9,   9,   9,  10,  10,  10,  10, 
   11,  11,  11,  11,  12,  12,  12,  12, 
   13,  13,  13,  14,  14,  14,  15,  15, 
   15,  16,  16,  16,  17,  17,  18,  18, 
   18,  19,  19,  20,  20,  21,  21,  22, 
   22,  23,  23,  24,  24,  25,  25,  26, 
   26,  27,  28,  28,  29,  30,  30,  31, 
   32,  32,  33,  34,  35,  35,  36,  37, 
   38,  39,  40,  40,  41,  42,  43,  44, 
   45,  46,  47,  48,  49,  51,  52,  53, 
   54,  55,  56,  58,  59,  60,  62,  63, 
   64,  66,  67,  69,  70,  72,  73,  75, 
   77,  78,  80,  82,  84,  86,  88,  90, 
   91,  94,  96,  98, 100, 102, 104, 107, 
  109, 111, 114, 116, 119, 122, 124, 127, 
  130, 133, 136, 139, 142, 145, 148, 151, 
  155, 158, 161, 165, 169, 172, 176, 180, 
  184, 188, 192, 196, 201, 205, 210, 214, 
  219, 224, 229, 234, 239, 244, 250, 255 
};





/*
*   Mandatory logic
 */
void blinkRed(void);
void blinkRed(uint8_t);

void setup() {
  pinMode(P1_0, OUTPUT);
  digitalWrite(P1_0, LOW);

  //ANALOG SETTINGS
  analogReference(INTERNAL1V5);
  analogRead(TEMPSENSOR); // first reading usually wrong
  //SPI SETTINGS
  
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(1); // MSB-first

  //RADIO SETTINGS
  radio.begin(RADIO_SPEED, RADIO_CHANNEL);  // Defaults 1Mbps, channel 0, max TX power
  radio.setRXaddress((void*)myAddress);
  radio.setTXaddress((void*)serverAddress);  
  radio.autoAck(true);
  
  radio.deepsleep();

  //LEDS Settings 
  pinMode(LED_DRIVER_R, OUTPUT);
  analogWrite(LED_DRIVER_R, 0);

#ifdef BIGCHIP
  #pragma message  ("Enabeling r and g")
  pinMode(LED_DRIVER_G, OUTPUT);
  analogWrite(LED_DRIVER_G, 0);
  pinMode(LED_DRIVER_B, OUTPUT);
  analogWrite(LED_DRIVER_B, 0);
#endif
 

  //Set current height so we dim the light at boot
  current[0] = 200;
  current[1] = 200;
  current[2] = 200;
  updateAnalog();
 
  blinkRed(5);
  delay(3000);
  
 
  pingServer();
  blinkRed();

  setPoint[0] = 0;
  setPoint[1] = 0;
  setPoint[2] = 0;
   
  radio.enableRX();  // Start listening
}


uint8_t packetCounter = 0x00;
uint8_t innerCounter = 0;


void loop() {
  char payLoad[32];
  radio.enableRX();

  if(radio.available(true))
  {     
    radio.read(payLoad); 
    networkHandler(payLoad);
  }


  /**
   *
   * Blink periodically and ping server
   *
   */

  //replacement of delay, to unblock cpu
  innerCounter++;
  if(innerCounter == 0xFF){
    innerCounter = 0;
    loopCounter++;
    
    //The Ping Routine
    if(loopCounter == TOPCOUNTER){
    //reset counter
    loopCounter = 0;
    blinkRed();
    pingServer();
    //blinkRed(2);
    //radio.enableRX();  // Start listening
    }
   
    for(int x=0;x<LEDLENGTH;x++)
    {
      if (current[x] < setPoint[x])
        current[x]++;
        
      else if (current[x] > setPoint[x])
        current[x]--;
    }
   updateAnalog();
  //}
  }
}
  

void updateAnalog(){
    //update the current value towards setpoint
    analogWrite(LED_DRIVER_R, linear_brightness_curve[current[0]]);
    #ifdef BIGCHIP
      #pragma message  ("Colors for bigchip updateAnalog")
      analogWrite(LED_DRIVER_G, linear_brightness_curve[current[1]]);
      analogWrite(LED_DRIVER_B, linear_brightness_curve[current[2]]);
    #endif
}


/**
 * Handels (re)transmision of the package to other nodes (if new)
 */
void  networkHandler(char inbuff[]){
  int status = 0;
  //Resent packet 
  if(inbuff[REG_PACK_TYPESET] == PACKET_RGB){
      status=1;
      // 3 bytes, they represent R,G,B
      setPoint[0] = inbuff[REG_PACK_VAL0] > MAXBRIGHNESS ? MAXBRIGHNESS : inbuff[REG_PACK_VAL0] ;
     #ifdef BIGCHIP
     #pragma message "network bigchip code"
      setPoint[1] = inbuff[REG_PACK_VAL1] > MAXBRIGHNESS ? MAXBRIGHNESS : inbuff[REG_PACK_VAL1] ;
      setPoint[2] = inbuff[REG_PACK_VAL2] > MAXBRIGHNESS ? MAXBRIGHNESS : inbuff[REG_PACK_VAL2] ;
    #endif  
    }
      //NOT A RGB PACKAGE!!!!
      else{
        status =10;
      }
   blinkRed(status);
}


void pingServer(){
 char d[7];
  d[REG_ADD1] = myAddress[REG_ADD1];
  d[REG_ADD2] = myAddress[REG_ADD2];
  d[REG_ADD3] = myAddress[REG_ADD3];
  d[REG_ADD4] = myAddress[REG_ADD4];
  d[REG_ADD5] = myAddress[REG_ADD5];
  d[REG_PACK_TYPESET] = PACKET_PING;        
  d[6] = '\0';
  
  radio.print(d);
  radio.flush();
 }


void blinkRed(uint8_t times){
  for (int x=0; x< times; x++){
    blink();
  }
}
void blinkRed()
{
  blink();
}

void blink(){
  digitalWrite(P1_0, HIGH);
  delay(BLINK_DELAY);
  digitalWrite(P1_0, LOW);
  delay(BLINK_DELAY);

}




