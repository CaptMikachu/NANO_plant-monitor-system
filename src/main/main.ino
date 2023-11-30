#include <Wire.h> // allows communication with I2C display
#include "rgb_lcd.h" // for controlling LCD-display
#include <math.h>
#include <TimerOne.h>

#define LCD_COLUMNS 16
#define LCD_ROWS 2

rgb_lcd lcd; // main display

struct Data { // digital values from each sensor
  uint16_t soil_top_d = 0;
  uint16_t soil_bottom_d = 0;
  uint16_t temp_air_d = 0;
  uint16_t temp_water_d = 0;
  uint16_t water_level_d = 0;
  uint16_t light_d = 0;
} Current_reading_digi;

Data data_arr[60]; //To hold data for 1 minute duration

uint8_t data_arr_idx = 0;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);

  Timer1.initialize(1000000); // call every 1 second(s)
  Timer1.attachInterrupt(timerOneISR); // TimerOne interrupt callback
  ADC_init();

  Serial.println("setup complete");
}

void loop() {
  greeting();
  while(true){
    main_menu();
  }
}

void greeting() {
  String greeting = "WELCOME!";
  uint8_t greeting_length = greeting.length();
  /*SCROLL FROM RIGHT TO CENTER*/
  for (uint8_t column = 16; column > ((((LCD_COLUMNS / 2)) - (greeting_length / 2)) - 1); column--) {
    lcd.setCursor(column, 0);
    lcd.print(greeting);
    if (column == ((((LCD_COLUMNS / 2)) - (greeting_length / 2)))) { // hold a little longer at the end
      delay(2000);
    }
    delay(300);
    lcd.clear();
  }
  delay(500);
}

void main_menu() {
  lcd.setCursor(0, 0);
  lcd.print("TEMP MOIST LIGHT");
}

void timerOneISR() {
  //Serial.println(Current_reading_digi.water_level_d);
  SREG &= ~(1 << 7); //Disable interrupts by writing 0 to 7th bit in SREG
  ADC_read();
  printout_formatter();
  SREG |= (1 << 7); //Enable interrupts by writing 1 to 7th bit in SREG
}

void printout_formatter() {
  /*Printout for digital values*/
  String digi_values_str =
  "====DIGITAL SENSOR READING===="
  "\nSoil moisture, top: " + String(Current_reading_digi.soil_top_d) +
  "\nSoil moisture, bottom: " + String(Current_reading_digi.soil_bottom_d) +
  "\nTemperature, air: " + String(Current_reading_digi.temp_air_d) +
  "\nTemperature, water: " + String(Current_reading_digi.temp_water_d) +
  "\nWater level: " + String(Current_reading_digi.water_level_d) +
  "\nLight amount: " + String(Current_reading_digi.light_d) +
  "\n////DIGITAL SENSOR READING END////\n";

  Serial.println(digi_values_str);
}

void ADC_init(){
  ADMUX |= (1 << REFS0); //To use internal reference voltage
  ADCSRA |= (1 << ADEN); //To enable adc
  ADCSRB = 0; //Status register B not needed

  /* DIDR0 disable analog pin's digital components for pins A0-3
     A4-5 used for I2C and A6-7 no digital components exist */
  DIDR0 |= (1 << ADC0D) | (1 << ADC1D) | (1 << ADC2D) | (1 << ADC3D);
  Serial.println("ADC initialization complete");
}

void ADC_read(){
  /* Read every analog pin excluding A4 and A5
     and store to data struct */
  uint8_t pins[] = {0, 1, 2, 3, 6, 7};
  for(int i = 0; i < (sizeof(pins)/sizeof(pins[0])); i++){
    ADMUX = (ADMUX & B11111000) | (pins[i] & B00000111);
    ADCSRA |= (1 << ADSC); //Start conversion
    while(ADCSRA & B01000000); //Wait for conversion to end
    switch(pins[i]){
      case 0: //Top soil moisture
        Current_reading_digi.soil_top_d = ADC;
        break;
      case 1: //Bottom soil moisture
        Current_reading_digi.soil_bottom_d = ADC;
        break;
      case 2: //Air temperature
        Current_reading_digi.temp_air_d = ADC;
        break;
      case 3: //Water temperature
        Current_reading_digi.temp_water_d = ADC;
        break;
      case 6: //Water level
        Current_reading_digi.water_level_d = ADC;
        break;
      case 7: //Light amount
        Current_reading_digi.light_d = ADC;
        break;
      default:
        break;
    } 
  }

  /* Store the current data struct
     to an array holding 60 data structs at a time
     so save data for a minute */
  data_arr[data_arr_idx] = Current_reading_digi;  
  if(data_arr_idx == 59){
    data_arr_idx = 0;
  }else{
    data_arr_idx++;
  }
}







