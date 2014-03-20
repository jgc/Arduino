// 2014-03-20
// Changes not uploaded
// changed led delay from 2000 to 500 ms
// report count corrected

#include <JeeLib.h>
#include <RF12.h>
MilliTimer readoutTimer, aliveTimer;
#define myNodeID 3      // RF12 node ID in the range 1-30
#define network 100      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module


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
int reportcount = 0;
int reportdelaymin = 1; // minutes
int reportdelay = 60 * reportdelaymin;

void setup () {
  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above
  rf12_easyInit(0);
 
  Serial.begin(57600);
  Serial.print("\n[radio ack demo]");
}
  
void loop () {
  if (aliveTimer.poll(60000))
        rf12_easySend(0, 0);
  if (reportcount >= reportdelay){
        temptx.motion = 0;
        temptx.code = 5;
        temptx.count = 0;
        rfwriteack();
        reportcount = 0;
  }  
  reportcount++;
  delay(1000);
}

//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//--------------------------------------------------------------------------------------------------
 static void rfwriteack(){
   vccRead();
   rf12_easyPoll();
   rf12_easySend(&temptx, 8); // two bytes for the battery reading, then 2*numSensors for the number of DS18B20s attached to Funky
}

//--------------------------------------------------------------------------------------------------
// Reads current voltage
//--------------------------------------------------------------------------------------------------
void vccRead(){
  temptx.supplyV = map(analogRead(6), 0, 1023, 0, 660);
}
