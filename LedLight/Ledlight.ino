#include "global_settings.h"

#include <Enrf24.h>
#include <nRF24L01.h>
#include <string.h>
#include <SPI.h>
#include <WS2811Driver.h>



#if defined(__MSP430G2452__) || defined(__MSP430G2553__) || defined(__MSP430G2231__) 
#else
#error Board not supported
#endif

#define CMD P2_0
#define CSN P2_1
#define IRQ P2_2

#define LED_RED P1_0
#define LED_CLK P2_3
#define LED_YELLOW P2_3
#define BLINK_DELAY 5
#define POWERNAP 5000

const unsigned char initColor[3] = {0xFE, 0x00, 0x00};

//ENABLE SERIAL AND PRINT DEBUGGIN
#define VERBOSE 1

Enrf24 radio(CMD, CSN, IRQ);
WS2811Driver ledStrip;

const uint8_t rxaddr[] = LED_CONTROLLER1;
const uint8_t txaddr[] = SERVER_ADDRESS;

void dump_radio_status_to_serialport(uint8_t);

uint8_t i = 0;
uint32_t average = 0;
uint32_t values[SAMPLES];

uint8_t moduloCnt=0;

void setup() {
#if VERBOSE > 0
  //SERIAL SETTINGS
  Serial.begin(9600);
  Serial.println("Beginning setup");
#endif
  //ANALOG SETTINGS
  analogReference(INTERNAL1V5);
  analogRead(TEMPSENSOR); // first reading usually wrong

  //SPI SETTINGS
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(1); // MSB-first
  
  //RADIO SETTINGS
  radio.begin(RADIO_SPEED, RADIO_CHANNEL);  // Defaults 1Mbps, channel 0, max TX power
  radio.setRXaddress((void*)rxaddr);
  radio.setTXaddress((void*)txaddr);  
#if VERBOSE > 0
  dump_radio_status_to_serialport(radio.radioState());
#endif
  radio.deepsleep();
  
  //LEDS Settings 
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW,LOW);
  
  //DIGITAL LED(S) Settings
  pinMode(LED_CLK, OUTPUT);
  ledStrip.begin();
  ledStrip.setPinMask(LED_CLK);
  ledStrip.setLEDCount(1);
  ledStrip.write(initColor, 3);
  
  //Blink redled to indicate 
  blinkRed();
#if VERBOSE > 0
  Serial.println("done Setup");
#endif

  
}

void blinkRed()
{
  digitalWrite(LED_RED, HIGH);
  delay(BLINK_DELAY);
  digitalWrite(LED_RED, LOW);
  delay(BLINK_DELAY);
}

void pingServer(){
   //send a package with Temperature
  char packetBuffer[8];
  packetBuffer[0] = rxaddr[0];
  packetBuffer[1] = rxaddr[1];
  packetBuffer[2] = rxaddr[2];
  packetBuffer[3] = rxaddr[3];
  packetBuffer[4] = rxaddr[4];
  packetBuffer[5] = PACKET_MYADDR;
  radio.print(packetBuffer);
  radio.flush();
}

void loop() {
  uint16_t cnt=0;
  char inbuf[32];
  blinkRed();
  pingServer();
  //Go listen
  radio.enableRX();  // Start listening
  
  //wait forever ;
   while (!radio.available(true))
   {
     cnt++;
     if (cnt == 0xFFFF)
       break;
   }
         
   if (radio.read(inbuf)) {
     
       // 3 bytes, they represent R,G,B
       //controllers take led as rbg :S
     unsigned char r,g,b,led[3];
     r=inbuf[0];
     g=inbuf[1];
     b=inbuf[2];
     led[0]=r;
     led[1]=b;
     led[2]=g;
     ledStrip.write(led, 3);
    }
  
  
   #if VERBOSE > 0
    dump_radio_status_to_serialport(radio.radioState());  // Should show Receive Mode
    Serial.print("waiting for POWERNAP seconds");
   #endif
   
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
 uint16_t adc = analogRead(A0); 
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
