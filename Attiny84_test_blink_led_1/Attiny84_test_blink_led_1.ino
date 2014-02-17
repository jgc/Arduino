int c = 0;
int LED = 10;

static void led (bool on) {
  digitalWrite(LED, on ? 0 : 1); // inverted logic
}

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);
}

void loop() {
  c++;
  for (int x = 0; x <= c; x++){
    led(true);
    delay(1000);
    led(false);
    delay(100);
  }
  if (c = 10){
    c = 0;
  }
}
