#define scaling_m 0.00686
#define scaling_b 0.589

// Pins
int pinArray[] = { 2, 3, 4, 5 };
int fsrAnalogPin = 0;
int sensorPins[] = { 8, 9, 10, 11, 12, 13 };

// Readings
int fsrReading;
int drivingMode;
int mode;
int distance = 0;
long duration = 0;

void read_sensor( int sensorPin ) {
  // TODO: Add in a moving average or something similar to smooth data
  duration = pulseIn( sensorPin, HIGH );
  distance = ( scaling_m * duration ) + scaling_b;

  //Serial.print( "S" );
  //Serial.print( sensorPin - sensorPins[0] );
  //Serial.print( ":" );
  //Serial.println( distance );
}

void stop() {
  // Set all motors to off
  for (int i = 0; i < (sizeof(pinArray)/sizeof(int)); i++){
    pinMode(pinArray[i],OUTPUT);
    digitalWrite(pinArray[i], LOW);    // initialise all relays to HIGH (light off)
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // Set FSR to read
  pinMode(fsrAnalogPin, INPUT);

  // Set all ultrasonic sensors to read
  for (int i = 0; i < (sizeof(sensorPins)/sizeof(int)); i++){
    pinMode(sensorPins[i], INPUT);
  }

  mode = 9;
  stop();
}

void loop() {
  // Writing FSR Data
  //fsrReading = analogRead( fsrAnalogPin );
  //Serial.print( "F1:" );
  //Serial.println( fsrReading );

  // Writing Ultrasonic Data
  //for (int i = 0; i < (sizeof(sensorPins)/sizeof(int)); i++){
  //  read_sensor( sensorPins[i] );
  //}

  // Reading Motor Control
  if (Serial.available() > 0) {
    mode = Serial.read() -'0';
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
