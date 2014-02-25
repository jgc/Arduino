/// @dir tinyTest
/// New version of the Room Node for the ATtiny85 (derived from rooms.pde).
// 2010-10-19 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// DS18B20 setup below + 4K7 between PWR and PIN 2 (data)
// No power control / power control to save battery
// JeeNode (pin 1 nearest to RFM12B)
// PIN - 1 GND  (Port 1, pin 3 of 6)                 / GND  (Port 1, pin 3 of 6)                 }
// PIN - 2 DI01 (Port 1, pin 2 of 6)  - coding pin 4 / AI01 (Port 1, pin 5 of 6} - coding pin 14  }
// PIN - 3 PWR  (Port 1, pin 1 of 6)                 / DI01 (Port 1, pin 2 of 6) - coding pin 4  }
//
// JeeNode Micro (pin 1 nearest to RFM12B)
// PIN - 1 GND  (Port 1, pin 5 of 8)                  / GND  (Port 1, pin 5 of 8)                 }
// PIN - 2 DI01 (Port 1, pin 4 of 8)  - coding pin 10 / AI01 (Port 1, pin 4 of 8} - coding pin 10  }
// PIN - 3 PWR  (Port 1, pin 3 of 8)                  / DI01 (Port 1, pin 1 of 8) - coding pin 8 }

// https://github.com/nathanchantrell/TinyTX/blob/master/TinyTX_DS18B20/TinyTX_DS18B20.ino
// https://github.com/mharizanov/TempTX_tiny/blob/master/TempTX_tiny.ino
// see http://jeelabs.org/2010/10/20/new-roomnode-code/
// and http://jeelabs.org/2010/10/21/reporting-motion/

// The complexity in the code below comes from the fact that newly detected PIR
// motion needs to be reported as soon as possible, but only once, while all the
// other sensor values are being collected and averaged in a more regular cycle.

#include <OneWire.h> // http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
#include <DallasTemperature.h> // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip
#include <JeeLib.h>
// #include "pins_arduino.h"
// #include <PortsSHT11.h>
#include <avr/sleep.h>
// #include <util/atomic.h>

#define BOOST 0   // measure battery on analog pin if 1, else vcc after

#define NODE 4  // wireless node ID to use for sending blips
#define GROUP  100   // wireless net group to use for sending blips
#define ID   1   // set this to a unique ID to disambiguate multiple nodes
#define SEND_MODE 3   // set to 3 if fuses are e=06/h=DE/l=CE, else set to 2

// JeeNode Micro - NO power save config (see above)
// #define ONE_WIRE_BUS 10

// JeeNode Micro - WITH power save config (see above)
#define ONE_WIRE_BUS 10
#define ONE_WIRE_POWER 8
#define PIR 9

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance
#define TEMPERATURE_PRECISION 12
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature

#define SEND_MODE 3   // set to 3 if fuses are e=06/h=DE/l=CE, else set to 2

static MilliTimer reportTimer;

struct {
  int temp; // temperature: -500..+500 (tenths)
  int lobat; // supply voltage dropped under 3.1V: 0..1
  int pirval; // PIR activated
  //byte boost;
} 
payload;

int startup = 0;

int numberOfDevices; // Number of temperature devices found
DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

volatile bool adcDone;

// for low-noise/-power ADC readouts, we'll use ADC completion interrupts
ISR(ADC_vect) { 
  adcDone = true; 
}

// this must be defined since we're using the watchdog for low-power waiting
ISR(WDT_vect) { 
  Sleepy::watchdogEvent(); 
}

static byte vccRead (byte count =4) {
  set_sleep_mode(SLEEP_MODE_ADC);
  // use VCC as AREF and internal bandgap as input
#if defined(__AVR_ATtiny84__)
  ADMUX = 33;
#else
  ADMUX = bit(REFS0) | 14;
#endif
  bitSet(ADCSRA, ADIE);
  while (count-- > 0) {
    adcDone = false;
    while (!adcDone)
      sleep_mode();
  }
  bitClear(ADCSRA, ADIE);  
  // convert ADC readings to fit in one byte, i.e. 20 mV steps:
  //  1.0V = 0, 1.8V = 40, 3.3V = 115, 5.0V = 200, 6.0V = 250
  return (55U * 1024U) / (ADC + 1) - 50;
  // return ADC;
}

// static void doReport() {
// }


void setup () {
    cli();
  CLKPR = bit(CLKPCE);
#if defined(__AVR_ATtiny84__)
  CLKPR = 0; // div 1, i.e. speed up to 8 MHz
#else
  CLKPR = 1; // div 2, i.e. slow down to 8 MHz
#endif
  sei();

#if defined(__AVR_ATtiny84__)
    // power up the radio on JMv3
    bitSet(DDRB, 0);
    bitClear(PORTB, 0);
#endif

  // setup for RFM12
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  delay(100);
  rf12_initialize(NODE, RF12_868MHZ, GROUP);
  // see http://tools.jeelabs.org/rfm12b
  rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V
  rf12_sleep(RF12_SLEEP);

  // payload.id = BLIP_ID;
  //payload.boost = BOOST;

  pinMode(PIR, INPUT);  
  payload.pirval = digitalRead(PIR);
  pinMode(ONE_WIRE_POWER, OUTPUT);
  digitalWrite(ONE_WIRE_POWER, HIGH);

  delay(100);
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
  for(int i=0;i<numberOfDevices; i++)
  {
    if(sensors.getAddress(tempDeviceAddress, i))
    {
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
    }
  }

  digitalWrite(ONE_WIRE_POWER, LOW); // turn DS18B20 power off

  MilliTimer t;
  while (!t.poll(1000))
    ;
}

void loop () {

  // payload.lobat = rf12_lowbat();
  // payload.lobat = map(analogRead(6), 0, 1023, 0, 6600); // millivolt - makes sketch too big
  payload.lobat = vccRead(); // see radioBlip2

  digitalWrite(ONE_WIRE_POWER, HIGH);
  delay(100);
  sensors.begin(); 
  sensors.requestTemperatures(); 
  delay(100);
  float tempC = sensors.getTempC(tempDeviceAddress);
  if (tempC > -20 && tempC < 51){
    float tempC1 = tempC + 0.05;
    int temp1 = 99;
    int tempX = (tempC * 100) / 10;
    int tempY = (tempC1 * 100) / 10;
    if (tempX == tempY) {
      temp1 = (tempC * 10);
    } 
    else {
      temp1 = (tempC1 * 10);
    }
    payload.temp = temp1;
    digitalWrite(ONE_WIRE_POWER, LOW);

    rf12_sleep(RF12_WAKEUP);
    rf12_sendNow(0, &payload, sizeof payload);
    rf12_sendWait(SEND_MODE);
    rf12_sleep(RF12_SLEEP);
    if (startup < 1){
      Sleepy::loseSomeTime(15000);
      ++startup;
    } else {
      Sleepy::loseSomeTime(60000);
      startup = 9;
    }
  }
}






