#include <JeeLib.h>
#include <PortsSHT11.h>
#include <avr/sleep.h>
#include <util/atomic.h>

int c = 0;
int LED = 10;
byte counter;

void setup() {
  //Serial.begin(38400);
  //Serial.print("\nSerial baud 38400");
  rf12_initialize(1, RF12_868MHZ, 100);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 1);
  delay(1000);
  digitalWrite(LED, 0);
  delay(2000);
}

void loop() {
  //Serial.print("Counter = ");
  //Serial.println(c);
  for (int x = 1; x <= c; x++){
    counter = B1;
    //rf12_sendNow(0, &counter, 1);
    //rf12_sendWait(1);
    digitalWrite(LED, 1);
    delay(500);
    counter = B0;
    //rf12_sendNow(0, &counter, 1);
    //rf12_sendWait(1);
    digitalWrite(LED, 0);
    delay(500);
  }
  c++;
  if (c == 5){
    c = 0;
  }
  delay(1500);
}

