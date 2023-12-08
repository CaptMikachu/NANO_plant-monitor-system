
uint8_t val1 = 10;
uint16_t val2 = 512;
uint8_t print1 = 0;
uint16_t print2 = 0;

void setup() {
  Serial.begin(9600);
  uint8_t bytes[2];
  uint8_t read_bytes[2];
  bytes[0] = (val2 >> 8) & 0xFF;
  bytes[1] = (val2 >> 0) & 0xFF;
  cli();
  //Write two values to eeprom, first is one byte and the second is two bytes
  while(EECR & (1 << EEPE));
  EEAR = 1;
  EEDR = val1;
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE);

  while(EECR & (1 << EEPE));
  EEAR = 2;
  EEDR = bytes[0];
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE);

  while(EECR & (1 << EEPE));
  EEAR = 3;
  EEDR = bytes[1];
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE);

  while(EECR & (1 << EEPE));
  EEAR = 1;
  EECR |= (1 << EERE);
  print1 = EEDR;

  while(EECR & (1 << EEPE));
  EEAR = 2;
  EECR |= (1 << EERE);
  read_bytes[0] = EEDR;

  while(EECR & (1 << EEPE));
  EEAR = 3;
  EECR |= (1 << EERE);
  read_bytes[1] = EEDR;
  print2 = (read_bytes[0] << 8) | read_bytes[1];
  Serial.println(read_bytes[0], BIN);
  Serial.println(read_bytes[1], BIN);
  Serial.print("One byte data: ");
  Serial.print(print1);
  Serial.print("\nTwo byte data: ");
  Serial.print(print2);
  sei();
}

void loop() {
}
