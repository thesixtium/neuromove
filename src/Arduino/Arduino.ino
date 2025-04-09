// Set all motors to off
void stop() {
  for (int i = 0; i < (sizeof(pinArray)/sizeof(int)); i++){
    pinMode(pinArray[i], OUTPUT);
    digitalWrite(pinArray[i], LOW);
  }
}

void setup() {
  Serial.begin(9600);
  stop();
}

void loop() {
  int mode;

  // Reading Motor Control
  if (Serial.available() > 0) {
    mode = Serial.read() - '0';
    stop();
    Serial.println(mode);

    if (mode == 52){
      digitalWrite(pinArray[0], HIGH);  // Right
    } else if (mode == 49){
      digitalWrite(pinArray[1], HIGH);  // Left
    } else if (mode == 67){
      digitalWrite(pinArray[2], HIGH);  // Reverse
    } else if (mode == 71){
      digitalWrite(pinArray[3], HIGH);  // Forward
    }
  }
}
