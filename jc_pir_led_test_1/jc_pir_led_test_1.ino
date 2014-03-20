#include <JeeLib.h>

Port pir (3);   // PIR sensor is connected to DIO3 (pin 2) of port 3
uint8_t state; 
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
  delay(2000);
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
  ledV = 1;
  blinkLed();
  ledV = 1;
  blinkLed();
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
      }
  }
}


