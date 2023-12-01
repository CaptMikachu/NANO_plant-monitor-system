#include <Wire.h> // allows communication with I2C display
#include "rgb_lcd.h" // for controlling LCD-display
#include <math.h>
#include <TimerOne.h>
#include <avr/wdt.h> //allows watchdog usage

#define LCD_COLUMNS 16
#define LCD_ROWS 2
#define VIEW_MAX 4

rgb_lcd lcd; // main display

volatile bool button_one_flag = 0; // Enter button
volatile bool button_two_flag = 0; // Exit button

bool button_one_state = 0;
bool button_two_state = 0;
bool button_three_state = 0;
bool button_four_state = 0;

uint8_t view_mode = 1;

struct Limits {
  uint16_t moisture_top = 0;
  uint16_t moisture_bottom = 0;
  uint16_t temp_air = 0;
  uint16_t water_level = 0;
} Limits;

struct Data { // digital values from each sensor
  uint16_t soil_top_d = 0;
  uint16_t soil_bottom_d = 0;
  uint16_t temp_air_d = 0;
  uint16_t temp_water_d = 0;
  uint16_t water_level_d = 0;
  uint16_t light_d = 0;
} Current_reading_digi;

struct ConvertData { // Converted values of each sensor
  char soil_top = 0;
  char soil_bottom = 0;
  uint8_t temp_air = 0;
  uint8_t temp_water = 0;
  uint8_t water_level = 0;
  uint16_t light = 0;
} Converted_current_reading;

ConvertData data_arr[60]; //To hold data for 1 minute duration

uint8_t data_arr_idx = 0;

void setup() {
  wdt_enable(WDTO_4S); //Enable watchdog using 4 second time
  Serial.begin(9600);
  lcd.begin(16, 2);

  Timer1.initialize(1000000); // call every 1 second(s)
  Timer1.attachInterrupt(timerOneISR); // TimerOne interrupt callback
  ADC_init();
  DDRD |= B10000000; // Pin 7 as ouput (water pump), other as input (buttons etc.)
  PORTD |= B11111100; // Water pump HIGH, input pullup to buttons
  attachInterrupt(digitalPinToInterrupt(2), button_one_ISR, FALLING); // Enter button
  attachInterrupt(digitalPinToInterrupt(3), button_two_ISR, FALLING); // Exit button
  wdt_reset();
  Serial.println("setup complete");
  //greeting();
}

void loop() {
  while(!button_one_flag && !button_two_flag){
    button_one_state = (PIND & B00000100);
    button_two_state = (PIND & B00001000);
    button_three_state = (PIND & B00010000);
    button_four_state = (PIND & B00100000);
    if ((millis() % 200) == 0) {
     main_menu();
    }
  }
  if (button_one_flag) {
    //settings();
    manual_watering();
  } else if (button_two_flag) {
    view_mode++;
    if (view_mode > VIEW_MAX) {
      view_mode = 1;
    }
  }
  button_one_flag = 0;
  button_two_flag = 0;
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
  String keys = "", values = "";
  switch (view_mode) {
    case 1:
      keys = "TEMP MOIST LIGHT";
      values =
      String(Converted_current_reading.temp_air) + " " +
      String(Converted_current_reading.soil_top) + " " +
      String(Converted_current_reading.light);
      break;
    case 2:
      keys = "MOIST-t  MOIST-b";
      values =
      String(Converted_current_reading.soil_top) + " " +
      String(Converted_current_reading.soil_bottom);
      break;
    case 3:
      keys = "TEMP-air  TEMP-w";
      values =
      String(Converted_current_reading.temp_air) + " " +
      String(Converted_current_reading.temp_water);
      break;
    case 4:
      keys = "TEMP-w   WATER-l";
      values =
      String(Converted_current_reading.temp_water) + " " +
      String(Converted_current_reading.water_level);
      break;
    default:
      keys = "TEMP MOIST LIGHT";
      values =
      String(Converted_current_reading.temp_air) + " " +
      String(Converted_current_reading.soil_top) + " " +
      String(Converted_current_reading.light);
      break;
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(keys);
  lcd.setCursor(0, 1);
  lcd.print(values);
}

void printout_formatter() {
  /*Printout for digital values*/
  String digi_values_str =
  "====DIGITAL SENSOR READING===="
  "\nSoil moisture, top: " + String(Converted_current_reading.soil_top) +
  "\nSoil moisture, bottom: " + String(Converted_current_reading.soil_bottom) +
  "\nTemperature, air: " + String(Converted_current_reading.temp_air) +
  "\nTemperature, water: " + String(Converted_current_reading.temp_water) +
  "\nWater level: " + String(Converted_current_reading.water_level) +
  "\nLight amount: " + String(Converted_current_reading.light) +
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
  wdt_reset();
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
    wdt_reset(); //Reset watchdog   
  }
  convert_data();
}

void convert_data(){ //Convert the current reading data to corresponding units

  float air_temp;
  float water_temp;
  float beta = 1.0 / 3435; //Beta value for thermistor
  float T0 = 1.0 / 298.15; //Reference temp for thermistor in kelvin
  float K;
  float R0 = 10000; //Resistance at 25C
  //Moisture conversion for top soil
  if(Current_reading_digi.soil_top_d < 300){
    Converted_current_reading.soil_top = 'L'; //Low moisture 
  }else if(Current_reading_digi.soil_top_d >= 300 && Current_reading_digi.soil_top_d < 700){
    Converted_current_reading.soil_top = 'M'; //Medium moisture
  }else{
    Converted_current_reading.soil_top = 'H'; //High moisture
  }
  
  //Moisture conversion for bottom soil
  if(Current_reading_digi.soil_bottom_d < 300){
    Converted_current_reading.soil_bottom = 'L'; //Low moisture 
  }else if(Current_reading_digi.soil_bottom_d >= 300 && Current_reading_digi.soil_bottom_d < 700){
    Converted_current_reading.soil_bottom = 'M'; //Medium moisture
  }else{
    Converted_current_reading.soil_bottom = 'H'; //High moisture
  }

  //Air temperature conversion
  air_temp = (float)Current_reading_digi.temp_air_d * (5/10.24);
  Converted_current_reading.temp_air = floor(air_temp);
  
  //Water temperature conversion (sensor is a thermistor)

  K = 1.0 / (T0 + beta*(log(1023.0 / (float)Current_reading_digi.temp_water_d - 1.00)));
  water_temp = K - 273.15;
  Converted_current_reading.temp_water = floor(water_temp);

  //Water level in cm(?)
  Converted_current_reading.water_level = (500.0 / 1023) * Current_reading_digi.water_level_d;

  //Light level (TODO define values for low, med, hi)
  Converted_current_reading.light = Current_reading_digi.light_d;

  data_arr[data_arr_idx] = Converted_current_reading; //Store the converted values from 1 minute duration
  if(data_arr_idx == 59){
    data_arr_idx = 0;
  }else{
    data_arr_idx++;
  }

}

void watering() {
  PORTD &= ~(1 << 7); // Turn on water pump
  delay(1000);
  PORTD |= (1 << 7); // Turn off water pump
}

void manual_watering() {
  PORTD &= ~(1 << 7); // Turn on water pump
  button_one_state = (PIND & B00000100);
  while (!button_one_state){
    button_one_state = (PIND & B00000100);
  };

  PORTD |= (1 << 7); // Turn off water pump
}

void settings() {
  wdt_disable();
  lcd.clear();
  while (!button_two_flag) {
    lcd.setCursor(0, 0);
    lcd.print("SETTINGS");
  }
  wdt_enable(WDTO_4S);
}

void timerOneISR() {
  ADC_read();
}

void button_one_ISR() {
  button_one_flag = 1;
}

void button_two_ISR() {
  button_two_flag = 1;
}
