#include <Enrf24.h>
#include <nRF24L01.h>
#include <string.h>
#include <SPI.h>

#include "global_settings.h"

#if defined(__MSP430G2452__) || defined(__MSP430G2553__) || defined(__MSP430G2231__) 
#else
#error Board not supported
#endif

#define CMD P2_0
#define CSN P2_1
#define IRQ P2_2

#define LED_RED RED_LED
#define LED_YELLOW P2_3


Enrf24 radio(CMD, CSN, IRQ);
const uint8_t rxaddr[] = SERVER_ADDRESS;
const uint8_t txaddr[] = SERVER_ADDRESS;
char aColour[3] = {0x00,0xFF,0xFF} ;

void dump_radio_status_to_serialport(uint8_t);


String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

#define VERBOSE 0

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
  radio.setRXaddress((void*)rxaddr);
 // radio.setTXaddress((void*)txaddr);  
  #if VERBOSE > 0
  dump_radio_status_to_serialport(radio.radioState());
  #endif
  
  radio.deepsleep();
  
  //LEDS Settings 
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW,LOW);
  
  //Blink redled to indicate 
  blinkRed();
  
  //Go listen
  radio.enableRX();  // Start listening
  
}

void blinkRed()
{
  digitalWrite(LED_RED, HIGH);
  delay(10);
  digitalWrite(LED_RED, LOW);
  delay(10);
}

void loop() {
  //read serial port:
 ProcessSerial();
  
   char inbuf[32];
    #if VERBOSE > 0
    dump_radio_status_to_serialport(radio.radioState());  // Should show Receive Mode
    Serial.print("Wainting for messages");
    #endif
    if (radio.available(true))
     if (radio.read(inbuf)) {
        #if VERBOSE > 0
        Serial.println("Received packet: ");
        Serial.print("byte 1: ");
       // printPackage(inbuf,8);
        
        
        #endif
      descisionMaker(inbuf);
    }
    
    if (stringComplete) {
      blinkRed();
      blinkRed();
      char address[5];
      char payload[4];
      //format: [address][action][value(s)];
      //format: [SERVA][0x01][value]
      address[0]=inputString[0];
      address[1]=inputString[1];
      address[2]=inputString[2];
      address[3]=inputString[3];
      address[4]=inputString[4];
      
      //set type of package
      payload[0]=inputString[5];
     
      switch(inputString[5])
      {
         case PACKET_BRIGHTNESS:
           payload[1] = inputString[6];
           break;
         
         case PACKET_SWITCH:
           payload[1] = inputString[6];
           break;
           
         case PACKET_RGB:
           payload[1] = inputString[6];
           payload[2] = inputString[7];
           payload[3] = inputString[8];
           break;
         
          default:
            Serial.print("Unknown type of package: ");
            Serial.print(inputString[6], HEX);
            break;
      }
      Serial.println("send command onAir;");
      
      
      //set package away
      sendRF(address, payload);
      inputString = "";
      stringComplete = false;
    }
}


/**
* This is excuted to decide how to convert the package to be pc-readable;
* payload-layout [0-4 address of the sending node][type ofpackage byte][payload]
*/
void descisionMaker(char *packet)
{
  blinkRed();
  char fromAddress[5];
   char c[1];
  #if VERBOSE > 0
    Serial.print("package type: ");
    Serial.print(packet[5],HEX);
    Serial.println("");
  #endif

  fromAddress[0] = packet[0];
  fromAddress[1] = packet[1];
  fromAddress[2] = packet[2];
  fromAddress[3] = packet[3];
  fromAddress[4] = packet[4];
  
  //the 5th byte contains type of package
  switch (packet[5]) {
   case PACKET_TEMP:
      uint16_t temperature;
      temperature = packet[6] | (uint16_t)packet[7] << 8;
      
      #if VERBOSE > 0
       Serial.println("lo/hi bytes: ");
       Serial.print(packet[6]);
       Serial.print(" - ");
       Serial.println(packet[7]);   
      printDec(temperature);
       
       #endif
       printToPcTemp(fromAddress, "t", temperature);
     
      break;
   
   case PACKET_BATT:
      uint32_t voltage;
    //  voltage = (packet[1] <<  24) | (packet[2] << 16) | (packet[3] << 8) | packet[4];
      #if VERBOSE > 0
      Serial.println("Voltage of battery: ");
      printDec(voltage);
      #endif
      break;
   
   case PACKET_MYADDR:
      printPing(fromAddress);
      break;
  
  case PACKET_BRIGHTNESS:
     
      c[0] = packet[6];
      printToPcNums(fromAddress, "b", c, 1);
      break;
      
      case PACKET_RGB:
      char brightness[3];
      brightness[0] = packet[6];
      brightness[1] = packet[7];
      brightness[2] = packet[8];
      printToPcNums(fromAddress, "b", brightness, 3);
      break;
 
  case PACKET_SWITCH:
      c[0] = packet[6];
      printToPcNums(fromAddress, "s", c, 1);
      break;
 
    
      
  default:
    //do nothing
    break;  
  }
   
}

void sendRF(char *address, char *data){
    radio.setTXaddress(address);
    radio.print(data);
    radio.flush();
}

void _printAddress(char *address){
    int x;  
    Serial.print("[");
    for (x=0;x<5;x++){
      Serial.print(address[x]);
    }
    Serial.print( "]");
}

void printToPcTemp(char * address, char * type, uint16_t value){
    _printAddress(address);
    Serial.print(type);
    Serial.print("=");
    Serial.print(value/10, DEC);
    Serial.print(".");
    Serial.print(value%10, DEC);
    Serial.print(";");
}

void printPing(char * address){
  _printAddress(address);
  Serial.print("p;");
}


void printToPcNums(char * address, char * type, char *vals, int length){
    int x;
    _printAddress(address);
    Serial.print(type);
    Serial.print("=");
    for (x=0;x<length;x++){
      Serial.print(vals[x], DEC);
    }
    Serial.print(";");
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
