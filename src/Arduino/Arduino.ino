#define scaling_m 0.00686
#define scaling_b 0.589

// Pins
int motorPinArray[] = { 2, 3, 4, 5 };
int fsrAnalogPin = 0;
int ultrasonicPinArray[] = { 7, 8, 9, 10, 11 };
int trigger = 6;

// Readings
int fsrReading;
int drivingMode;
int mode;
int distance = 0;
long duration = 0;

void stopMotors() {
  // Set all motors to off
  for (int i = 0; i < (sizeof(motorPinArray)/sizeof(int)); i++){
    pinMode(motorPinArray[i],OUTPUT);
    digitalWrite(motorPinArray[i], LOW);    // initialise all relays to HIGH (light off)
  }
}

void driveMotors(){
  if (Serial.available() > 0) {
    mode = Serial.read() -'0';
    stopMotors();
    Serial.println(mode);

    if (mode == 52){
      digitalWrite(motorPinArray[0], HIGH);  // Right
    } else if (mode == 49){
      digitalWrite(motorPinArray[1], HIGH);  // Left
    } else if (mode == 67){
      digitalWrite(motorPinArray[2], HIGH);  // Reverse
    } else if (mode == 71){
      digitalWrite(motorPinArray[3], HIGH);  // Forward
    }
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(19200);


  mode = 9;
  stopMotors();
}


void loop() {
  // Reading Motor Control
  driveMotors();

  // Loop Delay
  //delay(1000);
}
