int c1 = 0;
int LED = 10;

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
}

void loop() {
  c1++;
  digitalWrite(LED, 1);
  delay(1000);
  digitalWrite(LED, 0);
  Serial.print("Counter = ");
  Serial.println(c1);
  delay(1000);
}
