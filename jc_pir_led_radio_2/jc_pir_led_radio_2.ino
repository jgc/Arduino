// 2014-03-20
// Changes not uploaded
// changed led delay from 2000 to 500 ms
// report count corrected

#include <JeeLib.h>

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving
 
#define myNodeID 2      // RF12 node ID in the range 1-30
#define network 100      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module
#define pirdelay 60000       // Duration of sleep between measurements, in ms
 
//########################################################################################################################
//Data Structure to be sent, it is variable in size and we only send 2+n*2 bytes where n is the number of DS18B20 sensors attached
//########################################################################################################################
 
typedef struct {
  int supplyV;
  int motion; 
  int code;
  int count;
} Payload;
 
Payload temptx;

int numSensors;
 
Port pir (3);   // PIR sensor is connected to DIO3 (pin 2) of port 3
uint8_t state; 
int reportcount = 0;
int reportdelaymin = 5; // minutes
int reportdelay = 60 * reportdelaymin;
int pircount = 0;
int ledV = 0;
int LEDR = 7;
int LEDG = 17;
int POW = 16;

void blinkLed(){ 
  if (ledV == 0){
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDG, LOW);
  }
  if (ledV == 1){
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, HIGH);
  }
  if (ledV == 2){
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, LOW);
  }
  if (ledV == 3){
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDG, HIGH);
  }
  delay(500);
  digitalWrite(LEDR, LOW);
  digitalWrite(LEDG, LOW);
  ledV = 0;
}

void setup () {
  pinMode(POW, OUTPUT);
  digitalWrite(POW, HIGH);
  pinMode(LEDR, OUTPUT);
  digitalWrite(LEDR, HIGH);
  pinMode(LEDG, OUTPUT);
  digitalWrite(LEDG, HIGH);
  delay(1000);
  ledV = 1;
  blinkLed();
  ledV = 2;
  blinkLed();
  ledV = 2;
  blinkLed();
  delay(15000);
  ledV = 1;
  blinkLed();
  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above
  rf12_control(0xC000);           // Adjust low battery voltage to 2.2V
  rf12_sleep(0);                  // Put the RFM12 to sleep
 
  PRR = bit(PRTIM1);     // only keep timer 0 going
  ADCSRA &= ~ bit(ADEN); // Disable the ADC
  bitSet (PRR, PRADC);   // Power down ADC

  bitClear (ACSR, ACIE); // Disable comparitor interrupts
  bitClear (ACSR, ACD);  // Power down analogue comparitor

  Serial.begin(57600);
  Serial.print("\n[pir_demo]");
  pir.mode(INPUT);
  pir.mode2(INPUT);
  pir.digiWrite2(1); // pull-up

}
  
void loop () {
  if (pir.digiRead() != state) {
      state = pir.digiRead();
      Serial.print("\nPIR ");
      Serial.print(state ? "on " : "off");
      if (state == 1){
        ledV = 2;
        blinkLed();
        temptx.motion = 1;
        temptx.code = 9;
        temptx.count = pircount;
        rfwrite();
        ledV = 3;
        blinkLed();
        pircount++;
        if (pircount > 3){
          ledV = 1;
          blinkLed();
          //delay(30000);
          Sleepy::loseSomeTime(30000);
          pircount = 0;
        }
      }
  }
  reportcount++;
  Sleepy::loseSomeTime(1000);
  //delay(1000);
  if (reportcount >= reportdelay){
        temptx.motion = 0;
        temptx.code = 0;
        temptx.count = 0;
        rfwrite();
        reportcount = 0;
  }  
}
//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//--------------------------------------------------------------------------------------------------
 static void rfwrite(){
   rf12_sleep(-1);     //wake up RF module
   vccRead();
   while (!rf12_canSend())
   rf12_recvDone();
   //rf12_sendStart(0, &temptx, numSensors*2 + 2); // two bytes for the battery reading, then 2*numSensors for the number of DS18B20s attached to Funky
   rf12_sendStart(0, &temptx, 8, 1); // two bytes for the battery reading, then 2*numSensors for the number of DS18B20s attached to Funky
   //1 as 4th argument
   rf12_sendWait(2);    //wait for RF to finish sending while in standby mode
   rf12_sleep(0);    //put RF module to sleep
}
 
//--------------------------------------------------------------------------------------------------
// Reads current voltage
//--------------------------------------------------------------------------------------------------
void vccRead(){
  bitClear(PRR, PRADC); // power up the ADC
  ADCSRA |= bit(ADEN); // enable the ADC
  Sleepy::loseSomeTime(10);
  //delay(10);
  temptx.supplyV = map(analogRead(6), 0, 1023, 0, 660);
  ADCSRA &= ~ bit(ADEN); // disable the ADC
  bitSet(PRR, PRADC); // power down the ADC
}

long vccRead2() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  temptx.supplyV = result;
  //return result;
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
