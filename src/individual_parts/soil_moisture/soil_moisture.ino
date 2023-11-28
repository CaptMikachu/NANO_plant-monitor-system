uint16_t moisture;

void setup() {
  Serial.begin(9600);
  pinMode(A7, INPUT);

}

void loop() {
  moisture = analogRead(A7);
  Serial.println(moisture);
  delay(500);
}
