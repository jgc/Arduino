// 2014-03-20
// Changes not uploaded
// changed led delay from 2000 to 500 ms
// report count corrected

#define sketchV "Sketch: jc_pir_led_radio_ack_4.6"
#include <JeeLib.h>
#include <EEPROM.h>
#include <OneWire.h> // http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
#include <DallasTemperature.h> // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip
int debug1 = 1;
int eeAddr = 0;

ISR(WDT_vect) {
  Sleepy::watchdogEvent();  // interrupt handler for JeeLabs Sleepy power saving
}

#define myNodeID 3      // RF12 node ID in the range 1-30
#define network 100      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module
#define pirdelay 60000       // Duration of sleep between measurements, in ms
#define RADIO_SYNC_MODE 2
#define ACK_TIME 1000
#define NB_ATTEMPTS_ACK 4

int alarmPin = 9;

// DS18B20 WITH power save config (see above)
#define temp_BUS 10
#define temp_POW 8

OneWire oneWire(temp_BUS); // Setup a oneWire instance
#define temp_PREC 12
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature

//########################################################################################################################
//Data Structure to be sent, it is variable in size and we only send 2+n*2 bytes where n is the number of DS18B20 sensors attached
//########################################################################################################################

typedef struct {
  int supplyV;
  int motion;
  int code;
  int count;
  int netCode;
} Payload;

Payload temptx;

// separate black lead from front = power on momentary
// white lead on connector = 3.3v
// black lead on connector = GND
int numSensors;
int pirAttached = 0;
Port pir (3);   // PIR sensor is connected to DIO3 (pin 2) of port 3 - green lead on connector
uint8_t state;
int reportcount = 0;
int reportdelaymin = 1; // minutes
int reportdelay = 60 * reportdelaymin;
int pircount = 0;
int ledV = 0;
int LEDR = 7; // blue lead on connector
int LEDG = 17; // yellow lead on connector
int POW = 16; // black lead - direct
int RES = 14; // red lead on connector
int nAttempt = 1;
int valRES = 1;

void blinkLed() {
  if (ledV == 0) {
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDG, LOW);
  }
  if (ledV == 1) {
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, HIGH);
  }
  if (ledV == 2) {
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, LOW);
  }
  if (ledV == 3) {
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDG, HIGH);
  }
  delay(250);
  digitalWrite(LEDR, LOW);
  digitalWrite(LEDG, LOW);
  ledV = 0;
}

static byte waitForAck() {
	MilliTimer ackTimer;
	while (!ackTimer.poll(ACK_TIME)){
		if (rf12_recvDone() && rf12_crc == 0 && ((rf12_hdr & RF12_HDR_ACK) == 0) && ((rf12_hdr & RF12_HDR_CTL) == 128) ){
			Serial.print(" ACK Received. ");
			Serial.print("Node ID:");Serial.println(rf12_hdr & RF12_HDR_MASK); 
			//Serial.print("received something. ");
			//Serial.println(rf12_hdr);
			//Serial.print("RF12_HDR_DST=");Serial.println(rf12_hdr & RF12_HDR_DST);
			//Serial.print("RF12_HDR_CTL=");Serial.println(rf12_hdr & RF12_HDR_CTL);
			//Serial.print("RF12_HDR_ACK=");Serial.println(rf12_hdr & RF12_HDR_ACK);
			//Serial.print("rf12_len=");Serial.println(rf12_len);
                        //Serial.print("SupplyV: ");
                        //Serial.print(temptx.supplyV);
                        //Serial.print(" Motion: ");
                        //Serial.print(temptx.motion);
                        //Serial.print(" Code: ");
                        //Serial.print(temptx.code);
                        //Serial.print(" Count: ");
                        //Serial.print(temptx.count);
                        //Serial.print(" netCode: ");
                        //Serial.println(temptx.netCode);
		        delay(10);
                        return 1;
		}
	}
	return 0;
}


void setup () {
  Serial.begin(57600);
  Serial.println(sketchV);
  Serial.print("\n");
  pinMode(POW, OUTPUT);
  digitalWrite(POW, HIGH);
  pinMode(RES, INPUT);
  pinMode(LEDR, OUTPUT);
  digitalWrite(LEDR, HIGH);
  pinMode(LEDG, OUTPUT);
  digitalWrite(LEDG, HIGH);
  delay(1000);
  ledV = 1;
  blinkLed();
  delay(1000);
  ledV = 1;
  blinkLed();
  rf12_initialize(myNodeID, freq, network); // Initialize RFM12 with settings defined above
  rf12_easyInit(5);
  rf12_control(0xC000);           // Adjust low battery voltage to 2.2V
  rf12_sleep(0);                  // Put the RFM12 to sleep

  PRR = bit(PRTIM1);     // only keep timer 0 going
  ADCSRA &= ~ bit(ADEN); // Disable the ADC
  bitSet (PRR, PRADC);   // Power down ADC

  bitClear (ACSR, ACIE); // Disable comparitor interrupts
  bitClear (ACSR, ACD);  // Power down analogue comparitor

  pir.mode(INPUT);
  pir.mode2(INPUT);
  pir.digiWrite2(1); // pull-up

}

void loop () {
  //valRES = digitalRead(RES);
  //Serial.print("\nRES value: ");
  //Serial.println(valRES);
  if (valRES == 1){
    //Serial.println("Requested code?");
    delay(100);
    //valRES = digitalRead(RES);
    if (valRES == 1){
      digitalWrite(LEDR, HIGH);
      temptx.supplyV = 0;
      temptx.motion = 0;
      temptx.code = 99;
      Serial.println("Code requested ...");
      //delay(100);
      rfwrite();
      rf12_sendWait(4); 
      delay(100);
      Serial.println("Checking for Network Code");
      //delay(100);
      //===
      //if (rf12_recvDone() && rf12_crc == 0){
      if (rf12_recvDone()){
        Serial.println("\nCode Request Message Received . ");
	//int numSensors = rf12_len/2 - 1;
        const Payload* p = (const Payload*) rf12_data;
        Serial.print("Node: ");
        int node_id = (rf12_hdr & 0x1F); 
        Serial.print(node_id);
        Serial.print(" Voltage: ");
        Serial.print(p->supplyV / 100.);
        Serial.print(" Motion: ");
        Serial.print(p->motion);
        Serial.print(" Code: ");
        Serial.print(p->code);
        Serial.print(" count: ");
        Serial.print(p->count);
        Serial.print(" Netcode: ");
        Serial.print(p->netCode);
        Serial.println(); 
      }
      delay(100);
      Serial.println("End check for Network Code\n");
      //===
      digitalWrite(LEDR, LOW);
      valRES = 9;
      delay(100);
    } 
  }
  
  //delay(100);
  
  if (pir.digiRead() != state && pirAttached == 0) {
    state = pir.digiRead();
    //delay(10); //stop corruption of serial print
    //Serial.print("PIR ");
    //Serial.println(state ? "on " : "off");
    //delay(10); //stop corruption of serial print
    if (state == 1) {
      ledV = 2;
      blinkLed();
      temptx.motion = 1;
      temptx.code = 9;
      temptx.count = pircount;
      rfwrite();
      pircount++;
      Serial.print("PIR ");
      Serial.println(state ? "on " : "off");
      //int valRES = digitalRead(RES);
      if (pircount > 3){
        //ledV = 1;
        //blinkLed();
        //delay(30000);
        Serial.println("Count > 3 so loose some time");
        delay(10); //stop corruption of serial print
        if (debug1 == 0){
          Sleepy::loseSomeTime(30000);
        } else {
          Sleepy::loseSomeTime(1000);       
        }
        pircount = 0;
      }
    }
  }
  reportcount++;
  Sleepy::loseSomeTime(1000);
  delay(100);
  if (reportcount >= reportdelay) {
    Serial.println("\nRegular report ");
    temptx.motion = 0;
    temptx.code = 99;
    temptx.count = 0;
    rfwrite();
    if (nAttempt == NB_ATTEMPTS_ACK){
      Serial.println("Error sending");
    }
    reportcount = 0;
    valRES == 1;
      delay(100);
       //===
      if (rf12_recvDone() && rf12_crc == 0){
        Serial.println("Code Request Message Received. ");
	int numSensors = rf12_len/2 - 1;
        const Payload* p = (const Payload*) rf12_data;
        Serial.print("Node: ");
        int node_id = (rf12_hdr & 0x1F); 
        Serial.print(node_id);
        Serial.print(" Voltage: ");
        Serial.print(p->supplyV / 100.);
        if (numSensors>0) Serial.print(" Motion: ");
        if (numSensors>0) Serial.print(p->motion);
        if (numSensors>1) Serial.print(" Code: ");
        if (numSensors>1) Serial.print(p->code);
        if (numSensors>2) Serial.print(" count: ");
        if (numSensors>2) Serial.print(p->count);
        if (numSensors>3) Serial.print(" Netcode: ");
        if (numSensors>3) Serial.print(p->netCode);
        Serial.println(); 
      }
      //===
  }
}

//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//--------------------------------------------------------------------------------------------------
static void rfwriteack() {
  rf12_sleep(-1);     //wake up RF module
  vccRead();
  rf12_easyPoll();
  rf12_easySend(&temptx, 8); // two bytes for the battery reading, then 2*numSensors for the number of DS18B20s attached to Funky

  rf12_sendWait(4);    //wait for RF to finish sending while in standby mode
  rf12_sleep(0);    //put RF module to sleep
}

//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//--------------------------------------------------------------------------------------------------
static void rfwrite() {
  delay(10);
  rf12_sendWait(4); 
  Serial.println("Start rfsend ....");
  rf12_sleep(-1);     //wake up RF module
  vccRead();

  nAttempt = 1;
  bool flag_ACK_received = false;
  while (nAttempt < NB_ATTEMPTS_ACK && !flag_ACK_received ) {

    while (!rf12_canSend())
      rf12_recvDone();

    //rf12_sendStart(RF12_HDR_ACK, payload, sizeof payload);
    rf12_sendStart(RF12_HDR_ACK, &temptx, 10);
    rf12_sendWait(4);
    
    Serial.print("Attemps : "); 
    Serial.print(nAttempt);
    rf12_sendWait(4);
    
    if (waitForAck()) {
      //Serial.println("ACK received\n");
      //rf12_sendWait(4);
      flag_ACK_received = true;
      ledV = 3;
      blinkLed();
    } else {
      Serial.println(" ACK NOK received");
      delay(10);
      ledV = 1;
      blinkLed();
    }
    //rf12_sendWait(4);
    nAttempt++;
  }
  //rf12_sendWait(4);
  Serial.println("End rfsend");
  Serial.print("\n");
  delay(100);
  //rf12_sleep(0);    //put RF module to sleep
}

//--------------------------------------------------------------------------------------------------
// Reads current voltage
//--------------------------------------------------------------------------------------------------
void vccRead() {
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
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  temptx.supplyV = result;
  //return result;
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
