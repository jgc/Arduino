int led = 10;
int pon = 8;
int bp = 9;
int b1 = 7;
int b1v = 9;
int numTones = 10;
int tones[] = {261, 277, 294, 311, 330, 349, 370, 392, 415, 440};
// mid C C# D D# E F F# G G# A

void setup() {         
  Serial.begin(57600);   
  pinMode(pon, OUTPUT);
  digitalWrite(pon, HIGH);
  Serial.println("Power switch on");

  pinMode(led, OUTPUT);
  pinMode(b1, INPUT);

  delay(1000);
  digitalWrite(led, HIGH);
  pinMode(bp, OUTPUT);
  beep(100); 
  beep(100);
  delay(500);
  digitalWrite(led, 0);
  delay(500);
}

void loop() {
  b1v = digitalRead(b1);
  Serial.print("B1 value = ");
  Serial.println(b1v);
  if (b1v == 0){
    Serial.println("Beep 100");
    beep(100); 
    b1v = 9;
  }
  //toneloop();
  Serial.println("LED high");
  digitalWrite(led, HIGH);
  delay(1000);
  Serial.println("LED low");
  digitalWrite(led, LOW);
  delay(59000);
}

void beep(unsigned char delayms){
  analogWrite(bp, 20);      // Almost any value can be used except 0 and 255
                           // experiment to get the best tone
  delay(delayms);          // wait for a delayms ms
  analogWrite(bp, 0);       // 0 turns it off
  delay(delayms);          // wait for a delayms ms   
} 

void toneloop(){
for (int i = 0; i < numTones; i++)
  {
  tone(bp, tones[i]);
  delay(500);
  }
  noTone(bp);
}
