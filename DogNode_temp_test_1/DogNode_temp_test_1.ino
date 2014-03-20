#include <JeeLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
 
#define TEMPERATURE_PRECISION 9
#define ONE_WIRE_BUS 5  // pad 5 of the Funky
#define tempPower A0    // Power pin is connected pad 4 on the Funky
#define minutes 5       // Duration of sleep between measurements, in minutes
 
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
 
ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving
 
#define myNodeID 29      // RF12 node ID in the range 1-30
#define network 100      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module
 
//########################################################################################################################
//Data Structure to be sent, it is variable in size and we only send 2+n*2 bytes where n is the number of DS18B20 sensors attached
//########################################################################################################################
 
 typedef struct {
    int supplyV;    // Supply voltage
    int temp;   // Temperature reading
    int temp2;  // Temperature 2 reading
    int temp3;  // Temperature 3 reading
    int temp4;  // Temperature 4 reading
 } Payload;
 
 Payload temptx;
 
 int numSensors;

 int buzPin = 6;
 int ledPin = 7;
 int powPin = 8;
 int vcc;
 
//########################################################################################################################
 
void setup() {
  Serial.begin(57600);   
  pinMode(buzPin, OUTPUT); 
  digitalWrite(buzPin, HIGH); 
  pinMode(ledPin, OUTPUT); 
  digitalWrite(ledPin, HIGH); 
  pinMode(powPin, OUTPUT); 
  digitalWrite(powPin, HIGH); 
  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above
  rf12_control(0xC000);           // Adjust low battery voltage to 2.2V
  rf12_sleep(0);                          // Put the RFM12 to sleep
 
  PRR = bit(PRTIM1); // only keep timer 0 going
  ADCSRA &= ~ bit(ADEN); // Disable the ADC
  bitSet (PRR, PRADC);   // Power down ADC
  bitClear (ACSR, ACIE); // Disable comparitor interrupts
  bitClear (ACSR, ACD);  // Power down analogue comparitor
 
  pinMode(tempPower, OUTPUT);     // set power pin for DS18B20 to output
  digitalWrite(tempPower, HIGH);  // turn sensor power on
  Sleepy::loseSomeTime(50);       // Allow 50ms for the sensor to be ready
 
  sensors.begin();
  numSensors=sensors.getDeviceCount();
}
 
void loop() {
  pinMode(tempPower, OUTPUT); // set power pin for DS18B20 to output
  digitalWrite(tempPower, HIGH); // turn DS18B20 sensor on
  pinMode(buzPin, OUTPUT);
  digitalWrite(buzPin, HIGH); 
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH); 
  Sleepy::loseSomeTime(500);
  digitalWrite(buzPin, LOW); 
  pinMode(buzPin, INPUT);
  digitalWrite(ledPin, LOW);
  pinMode(buzPin, INPUT); 
  Sleepy::loseSomeTime(10); // Allow 10ms for the sensor to be ready
  sensors.requestTemperatures(); // Send the command to get temperatures
  temptx.temp=(sensors.getTempCByIndex(0)*100); // read sensor 1
  
  if (numSensors>1) temptx.temp2=(sensors.getTempCByIndex(1)*100); // read second sensor.. you may have multiple and count them upon startup but I only need two
  if (numSensors>2) temptx.temp3=(sensors.getTempCByIndex(2)*100);
  if (numSensors>3) temptx.temp4=(sensors.getTempCByIndex(3)*100);
  digitalWrite(tempPower, LOW); // turn DS18B20 sensor off
  pinMode(tempPower, INPUT); // set power pin for DS18B20 to input before sleeping, saves power
 
  vccRead();
  Serial.print("\nVcc / Vcc2 /-/ temp /-/ Mem : ");
  Serial.print(temptx.supplyV);
  Serial.print(" / ");
  Serial.print(readVcc2());
  Serial.print(" /-/ ");
  Serial.print(temptx.temp);
  Serial.print(" /-/ ");
  Serial.print(freeRam());
  Serial.flush();
  delay(15);
  //rfwrite(); // Send data via RF
 
  for(byte j = 0; j < minutes; j++) {    // Sleep for 5 minutes
    Sleepy::loseSomeTime(11473); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)
  }
 
}
 
//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//--------------------------------------------------------------------------------------------------
 static void rfwrite(){
   rf12_sleep(-1);     //wake up RF module
   while (!rf12_canSend())
   rf12_recvDone();
   rf12_sendStart(0, &temptx, numSensors*2 + 2); // two bytes for the battery reading, then 2*numSensors for the number of DS18B20s attached to Funky
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
  temptx.supplyV = map(analogRead(6), 0, 1023, 0, 660);
  ADCSRA &= ~ bit(ADEN); // disable the ADC
  bitSet(PRR, PRADC); // power down the ADC
}

long readVcc2() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
