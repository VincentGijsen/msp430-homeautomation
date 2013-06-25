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

#define VERBOSE 0

void setup() {
  //SERIAL SETTINGS
  Serial.begin(9600);

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
  delay(100);
}

void loop() {
   char inbuf[32];
    #if VERBOSE > 0
    dump_radio_status_to_serialport(radio.radioState());  // Should show Receive Mode
    Serial.print("Wainting for messages");
    #endif
    while (!radio.available(true))
      ;
    if (radio.read(inbuf)) {
        #if VERBOSE > 0
        Serial.println("Received packet: ");
        Serial.print("byte 1: ");
       // printPackage(inbuf,8);
        
        
        #endif
      descisionMaker(inbuf);
    }
}

 

void descisionMaker(char *packet)
{
  char fromAddress[5];
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
      blinkRed();
      uint16_t temperature;
      temperature = packet[6] | (uint16_t)packet[7] << 8;
      
      #if VERBOSE > 0
       Serial.println("lo/hi bytes: ");
       Serial.print(packet[6]);
       Serial.print(" - ");
       Serial.println(packet[7]);   
      printDec(temperature);
       
       #endif
       printData(fromAddress, "temp", temperature);
     
      break;
   
   case PACKET_BATT:
      blinkRed();
      blinkRed();
    
      uint32_t voltage;
    //  voltage = (packet[1] <<  24) | (packet[2] << 16) | (packet[3] << 8) | packet[4];
      #if VERBOSE > 0
      Serial.println("Voltage of battery: ");
      printDec(voltage);
      #endif
      break;
   
   case PACKET_MYADDR:
      blinkRed();
      blinkRed();
      blinkRed();
      blinkRed();
      printData(fromAddress, "Ping", 'p');

      SendColour(fromAddress, aColour);
      break;
  
  default:
    //do nothing
    break;  
  }
   
}

void SendColour(char *address, char *colors){
    radio.setTXaddress(address);
    radio.print(colors);
    radio.flush();
}

void printData(char * address, char * type, uint16_t value){
    int x;  
    Serial.print("[");
    for (x=0;x<5;x++){
      Serial.print(address[x]);
    }
    Serial.print( "]-");
    Serial.print(type);
    Serial.print("=");
    Serial.print(value/10, DEC);
    Serial.print(".");
    Serial.print(value%10, DEC);
    Serial.print(";");
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
