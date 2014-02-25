#include <JeeLib.h>
#include <PortsSHT11.h>
#include <avr/sleep.h>
#include <util/atomic.h>

int led = 10;
int pir = 8;
int pirval = 0;
byte payload = B0;

void setup() {
  pinMode(led, OUTPUT);
  pinMode(pir, INPUT);
  delay(1000);
  rf12_initialize(25, RF12_868MHZ, 100);

  digitalWrite(led, 1);
  delay(5000);
  digitalWrite(led, 0);
  delay(1000);
}

void loop() {
  pirval = digitalRead(pir);
  if (pirval == 1){
    payload = B1;
    rf12_sendNow(RF12_HDR_ACK, &payload, sizeof payload);
    digitalWrite(led, 1);
    delay(2000);
  } else {
    payload = B0;
    digitalWrite(led, 0);
    delay(100);
  }
}
