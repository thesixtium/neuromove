int rpiCurrentSensorPin = A0;
int rpiCurrentSensorValue = 0;
int rpiVoltageSensorPin = A1;
int rpiVoltageSensorValue = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.print(analogRead(rpiCurrentSensorPin));
  Serial.print(",")

  Serial.println(analogRead(rpiVoltageSensorPin));
}