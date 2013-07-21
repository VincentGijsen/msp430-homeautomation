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
//const uint8_t address[] = {'L','E','D','R','1'};
const uint8_t address[] = SERVER_ADDRESS;
uint8_t txaddress[] = SERVER_ADDRESS;

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

//used for resend after no-ack
boolean lastAck = false;
char lastCmd[10];
int timeoutCounter= 0;

//Prototype
void dump_radio_status_to_serialport(uint8_t);


void setup() {
  //SERIAL SETTINGS
  Serial.begin(9600);
  inputString.reserve(20);

  //SPI SETTINGS
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(1); // MSB-first
  
  //RADIO SETTINGS
  radio.begin(RADIO_SPEED, RADIO_CHANNEL);  // Defaults 1Mbps, channel 0, max TX power
  radio.setRXaddress((void*)address);
  radio.setTXaddress((void*)txaddress);
  radio.autoAck(true);
  
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
  /* 
   //retransmit code
  if (timeoutCounter >0){
    timeoutCounter-- ;
      if((!lastAck) && (timeoutCounter == 0))
      {//presume lastAck
        lastAck=true;
        radio.print(lastCmd);
        radio.flush();
        #if VERBOSE > 0
          Serial.println("did resend after timeout");
        #endif
      } 
   }
   */
    if (radio.available(true))
     if (radio.read(inbuf)) {
        #if VERBOSE > 0
        Serial.println("\nRAW PACKAGE: ");
        Serial.flush();
        for(int x=0;x<32;x++)
        {
          Serial.print(x, DEC);
          Serial.print(';');
          Serial.print(inbuf[x]);
          Serial.print('\n');
          Serial.flush();
          
        }
        Serial.flush();
        Serial.println("\n in hex:");
         for(int x=0;x<32;x++)
        { Serial.print(x, DEC);
          Serial.print(';');
          Serial.print(inbuf[x], HEX);
          Serial.print('\n');
          Serial.flush();
          
        }
        Serial.println(inbuf);
        Serial.flush();
        Serial.println("");
        #endif
        descisionMaker(inbuf);
      }
    
    
    if (stringComplete) {
      //timeoutCounter = RETRYCOUNTER;
      //lastAck = false;
      stringComplete = false;
      blinkYellow();
      char maxPackLeng = 32;
      char newPayload[maxPackLeng];
   
      for(int x=0;x<32;x++){
        newPayload[x] = inputString[x];
       if (inputString[x] == '\n'){
         break;
       } 
      }
        
        
        
      txaddress[0] = newPayload[REG_ADD1];
      txaddress[1] = newPayload[REG_ADD2];
      txaddress[2] = newPayload[REG_ADD3];
      txaddress[3] = newPayload[REG_ADD4];
      txaddress[4] = newPayload[REG_ADD5];
      
      radio.setTXaddress(txaddress);
      
      int retries = 10;
      while (retries != 0){
        blinkRed();
        radio.print(newPayload);
        radio.flush();
        boolean lastFailed = radio.isLastTXfailed();
    
        if (lastFailed){
         // Serial.println("Failed, retry send " + retries);
          retries --;  
          delay(20);
        }
        else{
             // Serial.println("success " + retries);
              break;
        }
      }
      
        inputString = "";
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
      
       #if VERBOSE > 1
      Serial.print("\nRGB: ");
      for (int x =0;x< 10;x++){
        Serial.print(packet[x], HEX);
        Serial.print(":");
      }
      #endif
      break;
 
  case PACKET_SWITCH:
      c[0] = packet[REG_PACK_VAL0];
      printToPcNums(packet, PACKET_SWITCH, c, 1);
      break;
 
   case PACKET_ACK:
       #if VERBOSE > 0
         Serial.println("got ack");
       #endif
       lastAck=true;
       printAck(packet);
       break;
      
  default:
    //do nothing
    break;  
  }
   
}


void _printAddress(char *address){
    Serial.print("[");
    Serial.print(address[REG_ADD1]);
    Serial.print(address[REG_ADD2]);
    Serial.print(address[REG_ADD3]);
    Serial.print(address[REG_ADD4]);
    Serial.print(address[REG_ADD5]);
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

void printAck(char * address){
  _printAddress(address);
  Serial.print("a;");
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
  while (Serial.available() > 0) {
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
