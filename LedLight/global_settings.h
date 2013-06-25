

//Overall settings

///NOTE SYMLINKED!

#ifndef GLOBAL_SETTINGS_H_
#define GLOBAL_SETTINGS_H_


#define SERVER_ADDRESS   {'M','A','H','U','B'}
#define SENSOR_ADDRESS_A {'S','E','N','S','A'}

#define LED_CONTROLLER1 {'L','E','D','C','1'}

#define RADIO_SPEED 1000000
#define RADIO_CHANNEL 125

#define PACKET_TEMP 0x01
#define PACKET_BATT 0x02
#define PACKET_MYADDR 0x03

#define SAMPLES 4


#endif /* SETTINGS */

#ifndef WS2811_H_
#define WS2811_H_

#define WS2811_BITMASK BIT7
#define WS2811_PORTDIR P2DIR
#define WS2811_PORTOUT P2OUT

#endif /* WS2811_H_ */

