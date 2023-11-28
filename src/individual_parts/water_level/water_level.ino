uint16_t water_level;

void setup() {
  Serial.begin(9600);
  pinMode(A7, INPUT);

}

void loop() {
  water_level = (500.0 / 1023) * analogRead(A7);
  Serial.println(water_level);
  delay(100);
}
