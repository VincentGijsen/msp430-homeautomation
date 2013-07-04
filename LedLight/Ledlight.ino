//Define if simple Dimmer, else RGB
//#define SIMPLETYPE true

#include "global_settings.h"

#include <Enrf24.h>
#include <nRF24L01.h>
#include <string.h>
#include <SPI.h>


//ENABLE SERIAL AND PRINT DEBUGGIN
#define VERBOSE 0

Enrf24 radio(CMD, CSN, IRQ);
const uint8_t addr[] = LISTEN_ADDRESS;
#ifdef SIMPLETYPE
const uint8_t myAddress[2] = C2;
#else
const uint8_t myAddress[2] = R1;
#endif

int8_t i = 0;
uint32_t average = 0;
uint32_t values[SAMPLES];

uint16_t loopCounter=0;

char setPoint[LEDLENGTH];
char current[LEDLENGTH];



/*
*  Prototypes
*/

void dump_radio_status_to_serialport(uint8_t);

/*
*   Mandatory logic
*/


void setup() {
  #if VERBOSE > 0
  //SERIAL SETTINGS
      Serial.begin(9600);
      Serial.println("Beginning setup");
  #endif
  //ANALOG SETTINGS
  analogReference(INTERNAL1V5);
  analogRead(TEMPSENSOR); // first reading usually wrong
  randomSeed(analogRead(3));
  //SPI SETTINGS
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(1); // MSB-first
  
  //RADIO SETTINGS
  radio.begin(RADIO_SPEED, RADIO_CHANNEL);  // Defaults 1Mbps, channel 0, max TX power
  radio.setRXaddress((void*)addr);
  radio.setTXaddress((void*)addr);  
  radio.autoAck(false);
  
  #if VERBOSE > 0
    dump_radio_status_to_serialport(radio.radioState());
  #endif
  radio.deepsleep();
  
  //LEDS Settings 
  pinMode(RED_LED, OUTPUT);
  #ifdef SIMPLETYPE
    pinMode(LED_DRIVER, OUTPUT);
    analogWrite(LED_DRIVER, 0);
  #else
    pinMode(LED_DRIVER_R, OUTPUT);
    pinMode(LED_DRIVER_G, OUTPUT);
    pinMode(LED_DRIVER_B, OUTPUT);
    
    analogWrite(LED_DRIVER_R, 0);
    analogWrite(LED_DRIVER_G, 0);
    analogWrite(LED_DRIVER_B, 0);
  #endif
  delay(1000);
  
  //Set current hight so we dim the light at boot
   current[0] = 30;
   #ifndef SIMPLETYPE
   current[1] = 30;
   current[2] = 30;
  #endif
  //Blink redled to indicate 
  blinkRed();
  pingServer();
  
  int x=0;
  for (x=0; x< LEDLENGTH; x++){
      setPoint[x] = 0;
  }
}

void blinkRed()
{
    digitalWrite(RED_LED, HIGH);
    delay(BLINK_DELAY);
    digitalWrite(RED_LED, LOW);
}

uint8_t packetCounter = random(0xFE);

void loop() {
    uint8_t x;
    char payLoad[32];
    //Go listen
    radio.enableRX();  // Start listening
    
    if(radio.available(true))
    {      
     if (radio.read(payLoad)) {
       handelPackage(payLoad);
     }
    }
  
  
     /**
    *
    * Blink periodically and ping server
    *
    */
    loopCounter++;
    if(loopCounter == TOPCOUNTER){
    //reset counter
        loopCounter = 0;
        blinkRed();
        pingServer();
    }
    
    if((loopCounter % FADEUPDATE) == 0)
    {
        for(x=0;x<LEDLENGTH;x++){
          if (current[x] < setPoint[x])
            current[x]++;
        else if (current[x] > setPoint[x])
            current[x]--;
    
        //update the current value towards setpoint
        #ifdef SIMPLETYPE
        analogWrite(LED_DRIVER, current[x]);
        #else
        analogWrite(LED_DRIVER_R, current[0]);
        analogWrite(LED_DRIVER_G, current[1]);
        analogWrite(LED_DRIVER_B, current[2]);
        #endif
    }
  }

delay(POWERNAP);  
}

void printPackage(char p[], int length){
    uint8_t it = 0;
    Serial.println("\npackage contence: ");

    for (it=0; it< length; it++){
        Serial.print(p[it], HEX);
        Serial.print("-");
    }

}

uint32_t readTemperature(){
// Formula: http://www.43oh.com/forum/viewtopic.php?p=18248#p18248
    for (i=0;i<SAMPLES;i++)
    {
        average -= values[i];
        values[i] = ((uint32_t)analogRead(TEMPSENSOR)*27069 - 18169625) *10 >> 16;
        average += values[i];
    } 
    return (average/SAMPLES);
}

uint16_t readBattery(){
    uint16_t adc = analogRead(A1); 
    uint32_t volt = (adc >> 10) * 1500;
    return volt; 
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

/**
  * Handels (re)transmision of the package to other nodes (if new)
  */ 
void  handelPackage(char *inbuff){
  //Resent packet 
  if(inbuff[REG_PACK_COUNTER] != packetCounter){
    //set packageCounter to new counter
    packetCounter = inbuff[REG_PACK_COUNTER];
    
    if(inbuff[REG_PACK_ADDRMSB] == myAddress[0] && inbuff[REG_PACK_ADDRLSB] == myAddress[1])
    {
     //We received a package for ourselves :)
     processPackage(inbuff);
    }
    else
    { 
          //This packet is not for us, but new, retransmit   
          //add some 'pseudo random sleep'
          delay(myAddress[0] + myAddress[1]);
          blinkRed();
          //broadcast old package with OLD counter!
          radio.print(inbuff);
          radio.flush();
          
    }          
    
   } 
}

/**
  *  Process the package as it is ours
  */
void processPackage(char *package){
#ifdef SIMPLETYPE
     //valid type of package
     if (package[REG_PACK_TYPESET] == PACKET_BRIGHTNESS){
       setPoint[0] = package[REG_PACK_VAL0];
       blinkRed();
     }
#else
     if(package[REG_PACK_TYPESET] == PACKET_RGB){
       // 3 bytes, they represent R,G,B
       setPoint[0] = package[REG_PACK_VAL0];
       setPoint[1] = package[REG_PACK_VAL1];
       setPoint[2] = package[REG_PACK_VAL2];
   }
#endif  
}

/**
  * Calcs new packetId 
  */
char calcPacketIdent(){
  if(packetCounter != 0xFF)
  {
    packetCounter++;
  }
  else
    packetCounter = 1;
    
  return packetCounter;
}

void pingServer(){
//send a package with Temperature
    char packetBuffer[4];
    packetBuffer[0] = calcPacketIdent();
    packetBuffer[1] = myAddress[0];    
    packetBuffer[2] = myAddress[1];
    packetBuffer[3] = PACKET_PING;
    radio.print(packetBuffer);
    radio.flush();
}



