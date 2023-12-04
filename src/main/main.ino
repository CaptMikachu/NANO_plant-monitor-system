#include <Wire.h> // allows communication with I2C display
#include "rgb_lcd.h" // for controlling LCD-display
#include <math.h>
#include <TimerOne.h>
#include <avr/wdt.h> //allows watchdog usage

#define LCD_COLUMNS 16
#define LCD_ROWS 2
#define VIEW_MAX 4

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

rgb_lcd lcd; // main display

volatile bool button_one_flag = 0; // Enter button
volatile bool button_two_flag = 0; // Exit button

bool button_one_state = 0;
bool button_two_state = 0;
bool button_three_state = 0;
bool button_four_state = 0;

uint8_t view_mode = 1;

struct Limits {
  uint16_t moisture_top_LOW = 300;
  uint16_t moisture_top_MED = 700;
  uint16_t moisture_bottom_HIGH = 700;
  uint8_t temp_air_MIN = 18;
  uint8_t temp_water_MIN = 18;
  int16_t water_level_LOW = 10;
  uint8_t water_bucket_height = 20;
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
    if (millis() % 10000 == 0) {
      limits_check();
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

void printout() {
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
  float K; // Temperature in Kelvins
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
  Converted_current_reading.temp_air = round(air_temp);
  
  //Water temperature conversion (sensor is a thermistor)

  K = 1.0 / (T0 + beta*(log(1023.0 / (float)Current_reading_digi.temp_water_d - 1.00)));
  water_temp = K - 273.15; // Convert Kelvins to Celsius
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
  delay(3000);
  PORTD |= (1 << 7); // Turn off water pump
}

void manual_watering() {
  PORTD &= ~(1 << 7); // Turn on water pump
  button_one_state = (PIND & B00000100);
  while (!button_one_state){
    button_one_state = (PIND & B00000100);
    delay(10);
  };

  PORTD |= (1 << 7); // Turn off water pump
  
}

void limits_check() {
  if (Current_reading_digi.soil_top_d < Limits.moisture_top_LOW &&
      Current_reading_digi.soil_bottom_d < Limits.moisture_bottom_HIGH) {
    watering();
  }
  if ((Limits.water_bucket_height - Converted_current_reading.water_level) < Limits.water_level_LOW) {
    tunes(1);
  }
  if (Converted_current_reading.temp_air < Limits.temp_air_MIN) {
    tunes(2);
  }
  if (Converted_current_reading.temp_water < Limits.temp_water_MIN) {
    tunes(3);
  }
}

void tunes(uint8_t tune_id) {
  switch (tune_id) {
    case 1: // LOW WATER LEVEL (cccp)
      hymn();
      break;
    case 2: // LOW AIR TEMPERATURE
      erika();
      break;
    case 3: // LOW WATER TEMPERATURE
      warning();
      break;
  }
}

void hymn() {
  Serial.println("CCCP");
  // notes in the melody
  int melody[] = {
  NOTE_G4, NOTE_C5, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_E4, NOTE_E4,
  NOTE_A4, NOTE_G4, NOTE_F4, NOTE_G4
  };
  // note durations: 4 = quarter note, 8 = eighth note, etc.:
  int noteDurations[] = {
  2, 2, 4, 4, 2, 4, 4, 2, 4, 4, 1
  };
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 13; thisNote++  ) {

  // to calculate the note duration, take one second
  // divided by the note type.
  //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 800/noteDurations[thisNote];
  tone(6, melody[thisNote],noteDuration);

  // to distinguish the notes, set a minimum time between them.
  // the note's duration   30% seems to work well:
  int pauseBetweenNotes = noteDuration * 1.30;
  delay(pauseBetweenNotes);
  // stop the tone playing:
  noTone(8);
  }
}

void erika() {
  Serial.println("ERIKA");
}

void warning() {
  Serial.println("WARNING");
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
