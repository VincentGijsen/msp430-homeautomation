#include <Enrf24.h>
#include <nRF24L01.h>
#include <string.h>
#include <SPI.h>

//
#define RED_LED P1_4
#define YELLOW_LED P2_3

#include "global_settings.h"


#define VERBOSE 0


Enrf24 radio(CMD, CSN, IRQ);
const uint8_t address[] = LISTEN_ADDRESS;


char aColour[3] = {0x00,0xFF,0xFF} ;

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

unsigned char packetCounter = 5;

//Prototype
void dump_radio_status_to_serialport(uint8_t);


void setup() {
  //SERIAL SETTINGS
  Serial.begin(9600);
  inputString.reserve(8);

  //SPI SETTINGS
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(1); // MSB-first
  
  //RADIO SETTINGS
  radio.begin(RADIO_SPEED, RADIO_CHANNEL);  // Defaults 1Mbps, channel 0, max TX power
  radio.setRXaddress((void*)address);
  radio.setTXaddress((void*)address);
  radio.autoAck(false);
  
 // radio.setTXaddress((void*)txaddr);  
  #if VERBOSE > 0
  dump_radio_status_to_serialport(radio.radioState());
  #endif
  
  radio.deepsleep();
  
  //LEDS Settings 
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED,LOW);
  
  //Blink redled to indicate 
  blinkRed();
  blinkYellow();
  
  //Go listen
  radio.enableRX();  // Start listening
  
}


void blinkYellow()
{
 digitalWrite(YELLOW_LED, HIGH);
  delay(5);
  digitalWrite(YELLOW_LED, LOW);
  
}
void blinkRed()
{
  digitalWrite(RED_LED, HIGH);
  delay(5);
  digitalWrite(RED_LED, LOW);
}

void loop() {
  //read serial port:
   ProcessSerial();
  
   char inbuf[32];
    #if VERBOSE > 0
    //dump_radio_status_to_serialport(radio.radioState());  // Should show Receive Mode
    //Serial.print("Wainting for messages");
    #endif
    if (radio.available(true))
     if (radio.read(inbuf)) {
        #if VERBOSE > 0
        Serial.println("Received packet: ");
        Serial.print("counter: ");
        Serial.println(inbuf[REG_PACK_COUNTER], HEX);
        Serial.println(inbuf);
        
        #endif
      if(inbuf[REG_PACK_COUNTER] != packetCounter){
        //Handle this NEW packa
        packetCounter = inbuf[REG_PACK_COUNTER];
        
        #if VERBOSE > 0
         Serial.print("new package :) old counter: ");
         Serial.print(packetCounter, HEX);
         Serial.print(" new: ");
         Serial.println(packetCounter, HEX);
        #endif
        
        descisionMaker(inbuf);
      }
      else
      {
        #if VERBOSE > 0
        Serial.println("Old package :(");
        //Old package
        #endif
      }
    }
    
    if (stringComplete) {
      blinkYellow();
      char newPayload[10];
      newPayload[REG_PACK_COUNTER]=calcPacketIdent();
      newPayload[REG_PACK_ADDRMSB]=inputString[0];
      newPayload[REG_PACK_ADDRLSB]=inputString[1];
      //set type of package
      newPayload[REG_PACK_TYPESET]=inputString[2];
     
      switch(newPayload[REG_PACK_TYPESET])
      {
         case PACKET_BRIGHTNESS:
           newPayload[REG_PACK_VAL0] = inputString[3];
           break;
         
         case PACKET_SWITCH:
           newPayload[REG_PACK_VAL0] = inputString[3];
           break;
           
         case PACKET_RGB:
           newPayload[REG_PACK_VAL0] = inputString[3];
           newPayload[REG_PACK_VAL1] = inputString[4];
           newPayload[REG_PACK_VAL2] = inputString[5];
           break;
         
          default:
            Serial.print("Unknown type of package: ");
            Serial.print(newPayload[REG_PACK_TYPESET], HEX);
            break;
      }
      
      #if VERBOSE
      Serial.print("send command onAir: ");
      for (int x =0;x< 10;x++){
        Serial.println(newPayload[x], HEX);
      }
      Serial.println(" ");
      Serial.print("pkg counter: ");
      Serial.print(newPayload[REG_PACK_COUNTER], HEX);
      
      #endif
      
      //set package away
      radio.print(newPayload);
      radio.flush();
      inputString = "";
      stringComplete = false;
      radio.enableRX();
    }
}


/**
* This is excuted to decide how to convert the package to be pc-readable;
* payload-layout [0-4 address of the sending node][type ofpackage byte][payload]
*/
void descisionMaker(char packet[])
{
  char c[1];
  blinkRed();
  #if VERBOSE > 0
    Serial.print("package type: ");
    Serial.println(packet[REG_PACK_TYPESET],HEX);
  #endif

  //the REG_PACK_TYPESET byte contains type of package
  switch (packet[REG_PACK_TYPESET]) {
   case PACKET_TEMP:
      uint16_t temperature;
      temperature = packet[REG_PACK_VAL0] | (uint16_t)packet[REG_PACK_VAL1] << 8;
      
      #if VERBOSE > 0
       Serial.println("lo/hi bytes: ");
       Serial.print(packet[REG_PACK_VAL0]);
       Serial.print(" - ");
       Serial.println(packet[REG_PACK_VAL1]);   
      printDec(temperature);
       
       #endif
       printToPcTemp(packet, ((char)REG_PACK_TYPESET), temperature);
     
      break;
   
   case PACKET_BATT:
      uint32_t voltage;
    //  voltage = (packet[1] <<  24) | (packet[2] << 16) | (packet[3] << 8) | packet[4];
      #if VERBOSE > 0
      Serial.println("Voltage of battery: ");
      printDec(voltage);
      #endif
      break;
   
   case PACKET_PING:
      printPing(packet);
      break;
  
  case PACKET_BRIGHTNESS:
     
      c[0] = packet[REG_PACK_VAL0];
      printToPcNums(packet, PACKET_BRIGHTNESS, c, 1);
      break;
      
   case PACKET_RGB:
      char brightness[3];
      brightness[0] = packet[REG_PACK_VAL0];
      brightness[1] = packet[REG_PACK_VAL1];
      brightness[2] = packet[REG_PACK_VAL2];
      printToPcNums(packet, PACKET_RGB, brightness, 3);
      break;
 
  case PACKET_SWITCH:
      c[0] = packet[REG_PACK_VAL0];
      printToPcNums(packet, PACKET_SWITCH, c, 1);
      break;
 
    
      
  default:
    //do nothing
    break;  
  }
   
}


/**
  * Calcs new packetId 
  */
unsigned char calcPacketIdent(){
  if(packetCounter <= 0xFA)
  {
    packetCounter = packetCounter + 3;
  }
  else
     packetCounter = 0x03;
  return packetCounter;
}


void _printAddress(char *address){
    Serial.print("[");
    Serial.print(address[REG_PACK_ADDRMSB]);
    Serial.print(address[REG_PACK_ADDRLSB]);
    Serial.print( "]");
}

void printToPcTemp(char * address, char type, uint16_t value){
    _printAddress(address);
    Serial.print(type);
    Serial.print("=");
    Serial.print(value/10, DEC);
    Serial.print(".");
    Serial.print(value%10, DEC);
    Serial.print(";");
    Serial.println();
    Serial.flush();
}

void printPing(char * address){
  _printAddress(address);
  Serial.print("p;");
  Serial.println();
  Serial.flush();
}


void printToPcNums(char * address, char type, char *vals, int length){
    int x;
    _printAddress(address);
    Serial.print(type);
    Serial.print("=");
    for (x=0;x<length;x++){
      Serial.print(vals[x], DEC);
    }
    Serial.print(";");
    Serial.println();
    Serial.flush();
}


void ProcessSerial() {
  if (Serial.available() > 0) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    } 
  }
}


#if VERBOSE > 0
void printDec(uint32_t ui) {
  Serial.print(ui/10, DEC);
  Serial.print(".");
  Serial.print(ui%10, DEC);
}

void dump_radio_status_to_serialport(uint8_t status)
{
  Serial.print("Enrf24 radio transceiver status: ");
  switch (status) {
    case ENRF24_STATE_NOTPRESENT:
      Serial.println("NO TRANSCEIVER PRESENT");
      break;

    case ENRF24_STATE_DEEPSLEEP:
      Serial.println("DEEP SLEEP <1uA power consumption");
      break;

    case ENRF24_STATE_IDLE:
      Serial.println("IDLE module powered up w/ oscillators running");
      break;

    case ENRF24_STATE_PTX:
      Serial.println("Actively Transmitting");
      break;

    case ENRF24_STATE_PRX:
      Serial.println("Receive Mode");
      break;

    default:
      Serial.println("UNKNOWN STATUS CODE");
  }
}

#endif
