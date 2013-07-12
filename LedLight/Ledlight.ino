//Define if simple Dimmer, else RGB

//#define SMALLCHIP true

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

const uint8_t myAddress[2] = {'L','A'};

int8_t i = 0;


uint16_t loopCounter=0;

char setPoint[3];
char current[3];


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
  radio.autoAck(false);
  
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
  current[0] = 100;
  current[1] = 100;
  current[2] = 100;
  updateAnalog();
 
  blinkRed(5);
   delay(3000);
  
 
  //Blink redled to indicate 
  // utils.blinkRed();
  pingServer();
  blinkRed();


  for (int x=0; x< 3; x++){
 current[x] = 0;
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

  innerCounter++;
  if(innerCounter == 0xFF){
    innerCounter = 0;
    loopCounter++;
  }
  
  if(loopCounter == TOPCOUNTER){
    //reset counter
    loopCounter = 0;
    blinkRed();
    pingServer();
    blinkRed(2);
    //radio.enableRX();  // Start listening
  }
  /*
  if((loopCounter % FADEUPDATE) == 0)
  {
    
    for(int x=0;x<3;x++){
      if (current[x] < setPoint[x])
        current[x]++;
      else if (current[x] > setPoint[x])
        current[x]--;
    }
   updateAnalog();
  }
  radio.isLastTXfailed();
  */
  
  updateAnalog();
  delay(POWERNAP);  
}

void updateAnalog(){
    //update the current value towards setpoint
    analogWrite(LED_DRIVER_R, current[0]);
    #ifdef BIGCHIP
      analogWrite(LED_DRIVER_G, current[1]);
      analogWrite(LED_DRIVER_B, current[2]);
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
      current[0] = inbuff[REG_PACK_VAL0];
      #ifdef BIGCHIP
        current[1] = inbuff[REG_PACK_VAL1];
        current[2] = inbuff[REG_PACK_VAL2];
      #endif
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
     // delay(myAddress[0] + myAddress[1]);
      //  blinkRed();
      //broadcast old package with OLD counter!
      
      //SET THE LENGH!!! if we walk outof your the memory, we'll crash!
      radio.write(inbuff);
      radio.flush();
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

void pingServer(){
  //send a package with Temperature
  char d[5];
  d[0] = calcPacketIdent();
  d[1] = myAddress[0];
  d[2] = myAddress[1];
  d[3] = PACKET_PING;
  d[4] = '\0';
  
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




