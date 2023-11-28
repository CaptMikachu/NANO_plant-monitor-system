/* Logic for moisture sensors
   to measure moisture on top 
   of soil and in the bottom */

uint16_t top_moist, bot_moist;

void setup() {
  DDRC = 0; //Configure all port c pins (analog pins) as input
  ADC_init();
  Serial.begin(9600);
}

void loop() {
  for(int i = 0; i < 2; i++){
    ADC_read(i);
  }
  Serial.println(top_moist);
  Serial.println(bot_moist);
  delay(1000);
}

void ADC_init(){
  /* Write 1 to REFS0 to use the 
    internal reference voltage,
    then write 1 to ADCSRA ADEN bit 
    to enable ADC. Make sure ADCSRB is all 0.
    Also disable analog pin's digital
    component by writing 1 to DIDR0 */
  ADMUX |= (1 << REFS0);
  ADCSRA |= (1 << ADEN);
  ADCSRB = 0;
  DIDR0 |= (1 << ADC0D) | (1 << ADC1D);
}

void ADC_read(int pin){
  /* Read the analog pin and convert
     value to digital */

  ADMUX = (ADMUX & B11111000) | (pin & B00001111);  //Choose channel with pin number
  ADCSRA |= (1 << ADSC); //Start ad conversion

  while(ADCSRA & B01000000); //Wait for the conversion bit to go down

  if(pin == 0){
    top_moist = ADC;
  }else if(pin == 1){
    bot_moist = ADC;
  }
}


