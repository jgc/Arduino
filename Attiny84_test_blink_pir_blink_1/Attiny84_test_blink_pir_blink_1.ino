int led = 10;
int pir = 8;
int pirval = 0;

void setup() {
  pinMode(led, OUTPUT);
  pinMode(pir, INPUT);
  digitalWrite(led, 1);
  delay(5000);
  digitalWrite(led, 0);
  delay(1000);
}

void loop() {
  pirval = digitalRead(pir);
  if (pirval == 1){
    digitalWrite(led, 1);
    delay(2000);
  } else {
    digitalWrite(led, 0);
    delay(100);
  }
}
