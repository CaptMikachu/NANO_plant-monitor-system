/* Logic for ultrasound sensor
   to measure water level height */


#define HEIGHT_SENSOR_PIN A6
uint16_t water_level;

void setup() {
  Serial.begin(9600);
  pinMode(HEIGHT_SENSOR_PIN, INPUT);

}

void loop() {
  water_level = (500.0 / 1023) * analogRead(HEIGHT_SENSOR_PIN);
  Serial.println(water_level);
  delay(100);
}
