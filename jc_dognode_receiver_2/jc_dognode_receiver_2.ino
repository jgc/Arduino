#include <JeeLib.h>
 
#define myNodeID 31      // RF12 node ID in the range 1-30
#define network 100      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module
 
int debug = 1; // 1 = debug
//########################################################################################################################
//Data Structure to be received
//########################################################################################################################

int alarmPin = 6;
int ledPin = 9;
int alarmc = 0;

 typedef struct {
    int supplyV;    // Supply voltage
    int temp;   // Temperature reading
    int temp2;  // Temperature 2 reading
    int temp3;  // Temperature 3 reading
    int temp4;  // Temperature 4 reading
 } Payload;
 
 Payload temptx;
 
 void setup(){
   Serial.begin(57600);
   Serial.println("Start");
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
   delay(5000);     
 }
 
 void loop(){
   if (rf12_recvDone() && rf12_crc == 0){
     int numSensors = rf12_len/2 - 1;
     const Payload* p = (const Payload*) rf12_data;
     Serial.print("Node: ");
     int node_id = (rf12_hdr & 0x1F); 
     Serial.print(node_id);
     Serial.print(" Voltage: ");
     Serial.print(p->supplyV / 100.);
     if (numSensors>0) Serial.print(" Motion: ");
     if (numSensors>0) Serial.print(p->temp);
     if (numSensors>1) Serial.print(" Code: ");
     if (numSensors>1) Serial.print(p->temp2);
     if (numSensors>2) Serial.print(" count: ");
     if (numSensors>2) Serial.print(p->temp3);
     if (numSensors>3) Serial.print(" Sensor4: ");
     if (numSensors>3) Serial.print(p->temp4 / 100.);
     Serial.println(); 
     if (RF12_WANTS_ACK){
        rf12_sendStart(RF12_ACK_REPLY, 0, 0);
        Serial.println("Acknowledgement sent ...");               
     }
     if ((p->temp == 1) && (p->temp2 == 9) && (p->temp3 > 1)){
       digitalWrite(alarmPin, HIGH);
       digitalWrite(ledPin, HIGH);
       Serial.println("Alarm activated");  
       delaytime(debug, 2);
       digitalWrite(alarmPin, LOW);
       digitalWrite(ledPin, LOW);
       }
     }
 }
 
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
