int c = 0;
int LED = 10;

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 1);
  delay(1000);
  digitalWrite(LED, 0);
  delay(5000);
}

void loop() {
  for (int x = 1; x <= c; x++){
    digitalWrite(LED, 1);
    delay(500);
    digitalWrite(LED, 0);
    delay(500);
  }
  c++;
  if (c == 5){
    c = 0;
  }
  delay(2500);
}

