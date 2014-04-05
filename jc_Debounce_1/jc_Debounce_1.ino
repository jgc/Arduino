// set pin numbers:
#define sketchV "Sketch: jc_Debounce_1"
const int buttonPin = 14;    // the number of the pushbutton pin
const int ledPin = 7;      // the number of the LED pin
const int ledPin2 = 17;      // the number of the LED pin
int count = 0;   

void setup() {
  Serial.begin(57600);
  Serial.println(sketchV);
  Serial.print("\n");
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin2, OUTPUT);

  // set initial LED state
  digitalWrite(ledPin, HIGH);
  digitalWrite(ledPin2, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
  digitalWrite(ledPin2, LOW);
  
}

void loop() {
  int reading = digitalRead(buttonPin);

  if (reading == 0){
    digitalWrite(ledPin, LOW);
    digitalWrite(ledPin2, HIGH);
    Serial.print("State / count: ");
    Serial.print(count);
    Serial.print(" / ");
    Serial.print(reading);
    Serial.println(" (Green)");
  }
  
  if (reading == 1){
    digitalWrite(ledPin, HIGH);
    digitalWrite(ledPin2, LOW);
    Serial.print("State / count: ");
    Serial.print(count);
    Serial.print(" / ");
    Serial.print(reading);
    Serial.println(" (Red)");  
}
  
  count++;
  if (count == 10){
   count = 1;
 }
  delay(1000);
}

