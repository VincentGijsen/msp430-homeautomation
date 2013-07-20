//Define if simple Dimmer, else RGB

//#define SMALLCHIP true

//0x40 == @ +1 > A
//#define PRINT_TOKEN(token) printf(#token " is %d", token)

#define MINOR 0x50
#define INDEX 1

#define LSBADDRESS (MINOR + INDEX)
#define ADRESSDELAY (LSBADDRESS - 20)
#include "global_settings.h"

#include <Enrf24.h>
#include <nRF24L01.h>
#include <string.h>
#include <SPI.h>
//#include <Utils.h>

//ENABLE SERIAL AND PRINT DEBUGGIN
#define VERBOSE 0

Enrf24 radio(CMD, CSN, IRQ);
const uint8_t addr[] = LISTEN_ADDRESS;

const uint8_t myAddress[2] = {'L', LSBADDRESS};
uint16_t loopCounter=0;

uint8_t setPoint[3];
uint8_t current[3];

unsigned char linear_brightness_curve[256] = { 
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
  radio.setRXaddress((void*)addr);
  radio.setTXaddress((void*)addr);  
  radio.autoAck(true);
  
  radio.deepsleep();

  //LEDS Settings 
  pinMode(LED_DRIVER_R, OUTPUT);
  analogWrite(LED_DRIVER_R, 0);

#ifdef BIGCHIP
  pinMode(LED_DRIVER_G, OUTPUT);
  analogWrite(LED_DRIVER_G, 0);
  pinMode(LED_DRIVER_B, OUTPUT);
  analogWrite(LED_DRIVER_B, 0);
#endif
 

  //Set current hight so we dim the light at boot
  current[0] = 255;
  current[1] = 255;
  current[2] = 255;
  updateAnalog();
 
  blinkRed(5);
  delay(3000);
  
 
  pingServer();
  blinkRed();


  for (int x=0; x< 3; x++){
   setPoint[x] = 0;
  }
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
    meshHandler(payLoad);
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
    
    //The fading of leds
     // if((loopCounter % FADEUPDATE) == 0)
     // {
    
      for(int x=0;x<3;x++){
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
      analogWrite(LED_DRIVER_G, linear_brightness_curve[current[1]]);
      analogWrite(LED_DRIVER_B, linear_brightness_curve[current[2]]);
    #endif
}


void printPackage(char p[], int length){
  uint8_t it = 0;
  Serial.println("\npackage contence: ");

  for (it=0; it< length; it++){
    Serial.print(p[it], HEX);
    Serial.print("-");
  }

}

/**
 * Handels (re)transmision of the package to other nodes (if new)
 */
void  meshHandler(char inbuff[]){
  int status = 0;
  //Resent packet 
  if(inbuff[REG_PACK_COUNTER] != packetCounter){
    //set packageCounter to new counter
    packetCounter = inbuff[REG_PACK_COUNTER];
    status = 1;
    if((inbuff[REG_PACK_ADDRMSB] == myAddress[0]) && (inbuff[REG_PACK_ADDRLSB] == myAddress[1]))
    {
      //We received a package for ourselves :)
      status=4;

     if(inbuff[REG_PACK_TYPESET] == PACKET_RGB){
      // 3 bytes, they represent R,G,B
      setPoint[0] = inbuff[REG_PACK_VAL0];
      #ifdef BIGCHIP
        setPoint[1] = inbuff[REG_PACK_VAL1];
        setPoint[2] = inbuff[REG_PACK_VAL2];
      #endif
      
      //send ACK to server
      ackServer();
      }
      else{
        //invalid packet for us!
        //BLINK LoNG!!!
        status =10;
      }
      
    }
    else
    { //The package is not for us, but we retransmit 1-time
      status = 2;
      //This packet is not for us, but new, retransmit   
      //add some 'pseudo random sleep'
      delay(ADRESSDELAY);
      //  blinkRed();
      //broadcast old package with OLD counter!
      
      //SET THE LENGH!!! if we walk out of our the memory, we'll crash!
      radio.write(inbuff);
      radio.flush();
      
      //retry if last tx failed to any-node
      if(radio.isLastTXfailed()){
         delay(ADRESSDELAY);
         radio.write(inbuff);
         radio.flush();
      }
    } 
    

  } 
  else{
    //We've seen this pakge already! 
    status = 1;
  }
  blinkRed(status);

}

 /* Calcs new packetId 
 */
char calcPacketIdent(){
  if(packetCounter < 0xF0)
  {
    packetCounter++;
  }
  else
    packetCounter = 0x01;

  return packetCounter;
}

void ackServer(){
   char d[8];
  d[0] = calcPacketIdent();
  d[1] = myAddress[0];
  d[2] = myAddress[1];
  d[3] = PACKET_ACK;
  d[4] = ';';
  d[5] = ';';
  d[6] = ';';
  d[7] = '\0';
  
  radio.print(d);
  radio.flush();
}

void pingServer(){
 char d[8];
  d[0] = calcPacketIdent();
  d[1] = myAddress[0];
  d[2] = myAddress[1];
  d[3] = PACKET_PING;
  d[4] = ';';
  d[5] = ';';
  d[6] = ';';
  d[7] = '\0';
  
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




