#define sketchV "Sketch: jc_dognode_receiver_5"

#include <JeeLib.h>
#include <EEPROM.h>

#define myNodeID 31      // RF12 node ID in the range 1-30
#define network 100      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module

#define RADIO_SYNC_MODE 2
#define ACK_TIME 1000
#define NB_ATTEMPTS_ACK 2  // was 4, just do it once
int nAttempt = 1;

int address = 0;
byte value;

#define lineBreak "=============================="
int debug = 1; // 1 = debug
//########################################################################################################################
//Data Structure to be received
//########################################################################################################################

int alarmPin = 6;
int ledPin = 9;
int alarmc = 0;
int sendNetCode = 0;

 typedef struct {
    int supplyV;    // Supply voltage
    int motion;   // Temperature reading
    int code;  // Temperature 2 reading
    int count;  // Temperature 3 reading
    int netCode;
 } Payload;
 
 Payload temptx;
 
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
                        return 1;
		}
	}
	return 0;
}
 
 void setup(){
   Serial.begin(57600);
   //delay(2000);
   Serial.print("Sketch: ");
   Serial.println(sketchV);
   Serial.print("NodeID: ");
   Serial.println(myNodeID);
   value = EEPROM.read(address);
   Serial.print("Network Code: ");  
   Serial.println(value, DEC); 
   Serial.println(lineBreak);
   rf12_initialize(myNodeID,freq,network);
   pinMode(alarmPin, OUTPUT);
   pinMode(ledPin, OUTPUT);
   
   Serial.println("Start up alarm");
   digitalWrite(alarmPin, HIGH);
   delaytime(debug, 1);
   digitalWrite(alarmPin, LOW);
   
   Serial.println("Start up LED");
   digitalWrite(ledPin, HIGH);
   delay(5000);
   digitalWrite(ledPin, LOW);
   
   Serial.println("Start up completed");  
   Serial.println(lineBreak);

   delay(5000);     
 }
 
 void loop(){
   // check if valid message received, if yes output
   if (rf12_recvDone() && rf12_crc == 0){
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
     value = p->netCode;
     EEPROM.write(address, value);
     Serial.println(); 
     
     // if message wants an acknowledgement send one
     if (RF12_WANTS_ACK && p->code != 99){
        rf12_sendStart(RF12_ACK_REPLY, 0, 0);
        Serial.println("Acknowledgement sent ...");               
     }

     // if message wants an acknowledgement send one
     if (RF12_WANTS_ACK && p->code == 99){
        sendNetCode = 1;
        rf12_sendStart(RF12_ACK_REPLY, 0, 0);
        Serial.println("Acknowledgement sent ...");               
     }
     
     // if motion = 1, code = 9 and count > 1 then sound alarm  
     if ((p->motion == 1) && (p->code == 9) && (p->count > 1)){
       digitalWrite(alarmPin, HIGH);
       digitalWrite(ledPin, HIGH);
       Serial.println("Alarm activated");  
       delaytime(debug, 2);
       digitalWrite(alarmPin, LOW);
       digitalWrite(ledPin, LOW);
     }
   }
   if (sendNetCode == 1){
     Serial.println("Send NetCode");  
     rfwrite();
     Serial.println("NetCode sent");  
     sendNetCode = 0; 
   }
   
 }
 
 // function to delay time based on debug mode
 void delaytime( int debug, int delaytype){
   if (debug == 0 && delaytype == 1){ 
     Serial.println("1ms alarm");  
     delay(10000);
   }
   if (debug == 0 && delaytype == 2){ 
     Serial.println("10s alarm");  
     delay(10000);
   }
   if (debug == 1){
     Serial.println("1ms alarm - debug");  
     delay(1);
   }
 }

//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//--------------------------------------------------------------------------------------------------
static void rfwrite() {
  //rf12_sleep(-1);     //wake up RF module
  vccRead();
  temptx.motion = 0;
  temptx.code = 5;
  temptx.count = 0;
  temptx.netCode = networkCode;
      
  nAttempt = 1;
  bool flag_ACK_received = false;
  while (nAttempt < NB_ATTEMPTS_ACK && !flag_ACK_received ) {

    while (!rf12_canSend())
      rf12_recvDone();

    //rf12_sendStart(RF12_HDR_ACK, payload, sizeof payload);
    rf12_sendStart(RF12_HDR_ACK, &temptx, 10);
    rf12_sendWait(4);
    
    Serial.print("Attempts : "); 
    Serial.print(nAttempt);
    rf12_sendWait(4);
    
    if (waitForAck()) {
      //Serial.println("ACK received\n");
      //rf12_sendWait(4);
      flag_ACK_received = true;
      //ledV = 3;
      //blinkLed();
    } else {
      Serial.println(" ACK NOT received");
      delay(10);
      //ledV = 1;
      //blinkLed();
    }
    //rf12_sendWait(4);
    nAttempt++;
  }
  //rf12_sendWait(4);
  Serial.print("\n");
  delay(2);
  rf12_sleep(0);    //put RF module to sleep
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
