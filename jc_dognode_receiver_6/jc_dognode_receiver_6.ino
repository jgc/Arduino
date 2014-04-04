#define sketchV "Sketch: jc_dognode_receiver_6"
#include <JeeLib.h>
#define networkCode 101
#define myNodeID 31      // RF12 node ID in the range 1-30
#define network 100      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module

MilliTimer timer;

#define RADIO_SYNC_MODE 2
#define ACK_TIME 1000
#define NB_ATTEMPTS_ACK 1 // was 4
int nAttempt = 1;
int node_id;
int nextId;
int sendCode = 0;

#define lineBreak "=============================="

int debug = 1; // 1 = debug

//########################################################################################################################
//Data Structure to be received
//########################################################################################################################

int alarmPin = 6;
int ledPin = 9;
int alarmc = 0;

 typedef struct {
    int supplyV;    // Supply voltage
    int motion;   // Temperature reading
    int code;  // Temperature 2 reading
    int count;  // Temperature 3 reading
    int netCode;
 } Payload;
 
 Payload temptx;
 
 void setup(){
   Serial.begin(57600);
   //delay(2000);
   Serial.print("Sketch: ");
   Serial.println(sketchV);
   Serial.print("NodeID: ");
   Serial.println(myNodeID);
   Serial.print("Network: ");
   Serial.println(network);
   Serial.print("Network code: ");
   Serial.println(networkCode);
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
   //Serial.println("Start of loop.");
   // check if valid message received, if yes output
   if (rf12_recvDone() && rf12_crc == 0){
     int numSensors = rf12_len/2 - 1;
     const Payload* p = (const Payload*) rf12_data;
     Serial.print("Node: ");
     node_id = (rf12_hdr & 0x1F); 
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
     
     // if message wants an acknowledgement send one
     if (RF12_WANTS_ACK && p->code != 99){
       temptx.supplyV = 0;
       temptx.motion = 0;
       temptx.code = 9;
       temptx.count = 0;
       temptx.netCode = networkCode;
       //rf12_sendWait(2);
       rf12_sendStart(RF12_ACK_REPLY, &temptx, 10);
       rf12_sendWait(4);
       Serial.println("Acknowledgement sent ...");               
     }
     
     if (RF12_WANTS_ACK && p->code == 99){
       temptx.supplyV = 0;
       temptx.motion = 0;
       temptx.code = 99;
       temptx.count = 0;
       temptx.netCode = networkCode;
       //rf12_sendWait(2);
       rf12_sendStart(RF12_ACK_REPLY, &temptx, 10);
       rf12_sendWait(4);
       Serial.println("Acknowledgement with code sent ...");      
     }
     
     if ((p->motion == 1) && (p->code == 9) && (p->count > 1)){
       digitalWrite(alarmPin, HIGH);
       digitalWrite(ledPin, HIGH);
       Serial.println("Alarm activated");  
       delaytime(debug, 2);
       digitalWrite(alarmPin, LOW);
       digitalWrite(ledPin, LOW);
       }
     
    }
 }
 

//========================================
// --- Functions -------------------------
//========================================

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
     delay(1000);
   }
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
