#include <Wire.h> // allows communication with I2C display
#include "rgb_lcd.h" // for controlling LCD-display
#include <math.h>
#include <TimerOne.h>
#include <avr/wdt.h> //allows watchdog usage

#define LCD_COLUMNS 16
#define LCD_ROWS 2
#define VIEW_MAX 4
#define SETTINGS_VIEW_MAX 7

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

unsigned long latest_time = 0;

volatile bool button_one_flag = 0; // Enter button
volatile bool button_two_flag = 0; // Exit button

bool button_one_state = 1;
bool button_two_state = 1;
bool button_three_state = 1;
bool button_four_state = 1;

uint8_t view_mode = 1;
uint8_t settings_view_mode = 1;

struct Limits {
  uint16_t moisture_top_LOW = 300;
  uint16_t moisture_top_HIGH = 700;
  uint16_t moisture_bottom_HIGH = 700;
  uint8_t temp_air_MIN = 18;
  uint8_t temp_water_MIN = 18;
  uint8_t water_level_LOW = 10;
  uint8_t water_bucket_height = 15;
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
  greeting();
}

void loop() {
  latest_time = millis();
  while((!button_one_flag && !button_two_flag) && (button_three_state && button_four_state)) {
    button_one_state = (PIND & B00000100);
    button_two_state = (PIND & B00001000);
    button_three_state = (PIND & B00010000);
    button_four_state = (PIND & B00100000);
    if ((millis() % 200) == 0) {
     main_menu();
    }
    if (millis() - latest_time > 10000) {
      limits_check();
      latest_time = millis();
    }
  }
  if (button_one_flag) {
    manual_watering();
  } else if (button_two_flag) {
    view_mode++;
    if (view_mode > VIEW_MAX) {
      view_mode = 1;
    }
  } else if (!button_three_state) {
    button_one_flag = 0;
    button_two_flag = 0;
    settings();
  } else if (!button_four_state) {
    button_one_flag = 0;
    button_two_flag = 0;
    printout();
  }

  button_one_flag = 0;
  button_two_flag = 0;
  button_one_state = (PIND & B00000100);
  button_two_state = (PIND & B00001000);
  button_three_state = (PIND & B00010000);
  button_four_state = (PIND & B00100000);
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

  //Ask user if they want to use default settings or saved settings after reset
  wdt_disable();
  lcd.setCursor(0, 0);
  lcd.print("Default settings");
  lcd.setCursor(0, 1);
  lcd.print("1.Yes   2.No");
  while(!button_one_flag && !button_two_flag); //Wait for user input
  if(button_two_flag){
    read_from_eeprom();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Settings loaded");
    delay(1000);
  }else{
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Default settings");
    delay(1000);
  }
  button_one_flag = 0;
  button_two_flag = 0;
  wdt_enable(WDTO_4S);
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
  /*Printout for converted values*/
  String digi_values_str =
  "====CONVERTED SENSOR READING===="
  "\nSoil moisture, top: " + String(Converted_current_reading.soil_top) +
  "\nSoil moisture, bottom: " + String(Converted_current_reading.soil_bottom) +
  "\nTemperature, air: " + String(Converted_current_reading.temp_air) +
  "\nTemperature, water: " + String(Converted_current_reading.temp_water) +
  "\nWater level: " + String(Converted_current_reading.water_level) +
  "\nLight amount: " + String(Converted_current_reading.light) +
  "\n////CONVERTED SENSOR READING END////\n";

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
    ADMUX = (ADMUX & B11111000) | (pins[i] & B00000111); //Enable the right admux channel

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
  float beta = 1.0 / 3435; //Beta value for thermistor, change to match your temperature sensor
  float T0 = 1.0 / 298.15; //Reference temp for thermistor in kelvin, change to match your temperature sensor
  float K; // Temperature in Kelvins
  float R0 = 10000; //Resistance at 25C, change to match your temperature sensor
  //Moisture conversion for top soil
  if(Current_reading_digi.soil_top_d < Limits.moisture_top_LOW){
    Converted_current_reading.soil_top = 'L'; //Low moisture 
  }else if(Current_reading_digi.soil_top_d >= Limits.moisture_top_LOW && Current_reading_digi.soil_top_d < Limits.moisture_top_HIGH){
    Converted_current_reading.soil_top = 'M'; //Medium moisture
  }else{
    Converted_current_reading.soil_top = 'H'; //High moisture
  }
  
  //Moisture conversion for bottom soil
  if(Current_reading_digi.soil_bottom_d < Limits.moisture_top_LOW){
    Converted_current_reading.soil_bottom = 'L'; //Low moisture 
  }else if(Current_reading_digi.soil_bottom_d >= Limits.moisture_top_LOW && Current_reading_digi.soil_bottom_d < Limits.moisture_bottom_HIGH){
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

  //Water level in cm (probably, should double check)
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
  int melody[] = {
  NOTE_G4, NOTE_GS4, NOTE_AS4, NOTE_DS2, NOTE_DS2, NOTE_DS2, NOTE_DS2,
  NOTE_DS2, NOTE_DS2, NOTE_C5, NOTE_AS4, NOTE_GS4
  };
  // note durations: 4 = quarter note, 8 = eighth note, etc.:
  int noteDurations[] = {
  16, 16, 16, 24, 64, 16, 24, 64, 8, 11, 44, 32
  };
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 13; thisNote++  ) {

  // to calculate the note duration, take one second
  // divided by the note type.
  //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 4600/noteDurations[thisNote];
  tone(6, melody[thisNote],noteDuration);

  // to distinguish the notes, set a minimum time between them.
  // the note's duration   30% seems to work well:
  int pauseBetweenNotes = noteDuration * 1.30;
  delay(pauseBetweenNotes);
  // stop the tone playing:
  noTone(6);
  }
}

void warning() {
  Serial.println("WARNING");
}


void settings() {
  wdt_disable();
  button_one_flag = 0;
  button_two_flag = 0;
  String view_name;
  latest_time = millis();
  while (!button_one_flag) {
    button_four_state = (PIND & B00100000);
    if (button_two_flag) {
      settings_view_mode++;
      button_two_flag = 0;
      if (settings_view_mode > SETTINGS_VIEW_MAX) {
        settings_view_mode = 1;
      }
    }
    switch (settings_view_mode) {
      case 1:
        view_name = "MOIST-top LOW";
        break;
      case 2:
        view_name = "MOIST-top MED";
        break;
      case 3:
        view_name = "MOIST-bot. HIGH";
        break;
      case 4:
        view_name = "TEMP-air MIN";
        break;
      case 5:
        view_name = "TEMP-water MIN";
        break;
      case 6:
        view_name = "Water level LOW";
        break;
      case 7:
        view_name = "Water bucket h.";
        break;
      default:
        view_name = "MOIST-top LOW";
        break;
    }
    if (millis() - latest_time > 300) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SETTINGS");
      lcd.setCursor(0, 1);
      lcd.print(view_name);
      latest_time = millis();
    }
    if (!button_four_state) {
      adjust_limits(settings_view_mode);
    }
  }
  //Save settings to EEPROM here
  save_to_eeprom();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SETTINGS SAVED!");
  delay(1000);
  lcd.clear();
  button_one_flag = 0;
  button_two_flag = 0;
  button_one_state = (PIND & B00000100);
  button_two_state = (PIND & B00001000);
  button_three_state = (PIND & B00010000);
  button_four_state = (PIND & B00100000);
  wdt_enable(WDTO_4S);
}



void adjust_limits(uint8_t limit_id) {
  switch (limit_id) {
      case 1:
        adjust_moisture_top_LOW();
        break;
      case 2:
        adjust_moisture_top_HIGH();
        break;
      case 3:
        adjust_moisture_bottom_HIGH();
        break;
      case 4:
        adjust_temp_air_MIN();
        break;
      case 5:
        adjust_temp_water_MIN();
        break;
      case 6:
        adjust_water_level_LOW();
        break;
      case 7:
        adjust_water_bucket_height();
        break;
      default:
        adjust_moisture_top_LOW();
        break;
    }
}

void adjust_moisture_top_LOW() {
  delay(200); // To let button state reset because of the debounce capacitor
  button_one_flag = 0;
  button_two_flag = 0;
  while (!button_one_flag) {
    if (millis() - latest_time > 500) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MOIST-top LOW");
      lcd.setCursor(0, 1);
      lcd.print(Limits.moisture_top_LOW);
      latest_time = millis();
    }
    button_three_state = (PIND & B00010000);
    button_four_state = (PIND & B00100000);
    if (!button_three_state) {
      Limits.moisture_top_LOW -= 50;
      if (Limits.moisture_top_LOW < 100) {
        Limits.moisture_top_LOW = 500;
      }
    } else if (!button_four_state) {
      Limits.moisture_top_LOW += 50;
      if (Limits.moisture_top_LOW > 500) {
        Limits.moisture_top_LOW = 100;
      }
    }
    while (!((PIND & B00100000) && (PIND & B00010000))); // To let button state reset because of the debounce capacitor
  }
}

void adjust_moisture_top_HIGH() {
  delay(200); // To let button state reset because of the debounce capacitor
  button_one_flag = 0;
  button_two_flag = 0;
  while (!button_one_flag) {
    if (millis() - latest_time > 500) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MOIST-top HIGH");
      lcd.setCursor(0, 1);
      lcd.print(Limits.moisture_top_HIGH);
      latest_time = millis();
    }
    button_three_state = (PIND & B00010000);
    button_four_state = (PIND & B00100000);
    if (!button_three_state) {
      Limits.moisture_top_HIGH -= 50;
      if (Limits.moisture_top_HIGH < 550) {
        Limits.moisture_top_HIGH = 900;
      }
    } else if (!button_four_state) {
      Limits.moisture_top_HIGH += 50;
      if (Limits.moisture_top_HIGH > 900) {
        Limits.moisture_top_HIGH = 550;
      }
    }
    while (!((PIND & B00100000) && (PIND & B00010000))); // To let button state reset because of the debounce capacitor
  }
}

void adjust_moisture_bottom_HIGH() {
  delay(200); // To let button state reset because of the debounce capacitor
  button_one_flag = 0;
  button_two_flag = 0;
  while (!button_one_flag) {
    if (millis() - latest_time > 500) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MOIST-bot. HIGH");
      lcd.setCursor(0, 1);
      lcd.print(Limits.moisture_bottom_HIGH);
      latest_time = millis();
    }
    button_three_state = (PIND & B00010000);
    button_four_state = (PIND & B00100000);
    if (!button_three_state) {
      Limits.moisture_bottom_HIGH -= 50;
      if (Limits.moisture_bottom_HIGH < 550) {
        Limits.moisture_bottom_HIGH = 1000;
      }
    } else if (!button_four_state) {
      Limits.moisture_bottom_HIGH += 50;
      if (Limits.moisture_bottom_HIGH > 1000) {
        Limits.moisture_bottom_HIGH = 550;
      }
    }
    while (!((PIND & B00100000) && (PIND & B00010000))); // To let button state reset because of the debounce capacitor
  }
}

void adjust_temp_air_MIN() {
  delay(200); // To let button state reset because of the debounce capacitor
  button_one_flag = 0;
  button_two_flag = 0;
  while (!button_one_flag) {
    if (millis() - latest_time > 500) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("TEMP-air MIN");
      lcd.setCursor(0, 1);
      lcd.print(Limits.temp_air_MIN);
      latest_time = millis();
    }
    button_three_state = (PIND & B00010000);
    button_four_state = (PIND & B00100000);
    if (!button_three_state) {
      Limits.temp_air_MIN -= 1;
      if (Limits.temp_air_MIN < 1) {
        Limits.temp_air_MIN = 50;
      }
    } else if (!button_four_state) {
      Limits.temp_air_MIN += 1;
      if (Limits.temp_air_MIN > 50) {
        Limits.temp_air_MIN = 1;
      }
    }
    while (!((PIND & B00100000) && (PIND & B00010000))); // To let button state reset because of the debounce capacitor
  }
}

void adjust_temp_water_MIN() {
  delay(200); // To let button state reset because of the debounce capacitor
  button_one_flag = 0;
  button_two_flag = 0;
  while (!button_one_flag) {
    if (millis() - latest_time > 500) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("TEMP-water MIN");
      lcd.setCursor(0, 1);
      lcd.print(Limits.temp_water_MIN);
      latest_time = millis();
    }
    button_three_state = (PIND & B00010000);
    button_four_state = (PIND & B00100000);
    if (!button_three_state) {
      Limits.temp_water_MIN -= 1;
      if (Limits.temp_water_MIN < 1) {
        Limits.temp_water_MIN = 40;
      }
    } else if (!button_four_state) {
      Limits.temp_water_MIN += 1;
      if (Limits.temp_water_MIN > 40) {
        Limits.temp_water_MIN = 1;
      }
    }
    while (!((PIND & B00100000) && (PIND & B00010000))); // To let button state reset because of the debounce capacitor
  }
}

void adjust_water_level_LOW() {
  delay(200); // To let button state reset because of the debounce capacitor
  button_one_flag = 0;
  button_two_flag = 0;
  while (!button_one_flag) {
    if (millis() - latest_time > 500) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Water level LOW");
      lcd.setCursor(0, 1);
      lcd.print(Limits.water_level_LOW);
      latest_time = millis();
    }
    button_three_state = (PIND & B00010000);
    button_four_state = (PIND & B00100000);
    if (!button_three_state) {
      Limits.water_level_LOW -= 2;
      if (Limits.water_level_LOW < 10) {
        Limits.water_level_LOW = 90;
      }
    } else if (!button_four_state) {
      Limits.water_level_LOW += 2;
      if (Limits.water_level_LOW > 90) {
        Limits.water_level_LOW = 10;
      }
    }
    while (!((PIND & B00100000) && (PIND & B00010000))); // To let button state reset because of the debounce capacitor
  }
}

void adjust_water_bucket_height() {
  delay(200); // To let button state reset because of the debounce capacitor
  button_one_flag = 0;
  button_two_flag = 0;
  while (!button_one_flag) {
    if (millis() - latest_time > 500) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Water bucket h.");
      lcd.setCursor(0, 1);
      lcd.print(Limits.water_bucket_height);
      latest_time = millis();
    }
    button_three_state = (PIND & B00010000);
    button_four_state = (PIND & B00100000);
    if (!button_three_state) {
      Limits.water_bucket_height -= 5;
      if (Limits.water_bucket_height < 15) {
        Limits.water_bucket_height = 100;
      }
    } else if (!button_four_state) {
      Limits.water_bucket_height += 5;
      if (Limits.water_bucket_height > 100) {
        Limits.water_bucket_height = 15;
      }
    }
    while (!((PIND & B00100000) && (PIND & B00010000))); // To let button state reset because of the debounce capacitor
  }
}

void save_to_eeprom(){
  cli(); //Disable interrupts to make sure eeprom writing is not corrupted
  EEAR = 0; //Make sure the EEPROM address register is all 0
  uint8_t bytes[2]; //Array to hold the value of variables bigger than 1 byte

  //Top moisture sensor low threshold
  //First we have to split the uint16_t datas to separate bytes
  bytes[0] = (Limits.moisture_top_LOW >> 8) & 0xFF; //Shift the first eight bits starting from left and then AND them with 0xFF which equals to 255
  bytes[1] = (Limits.moisture_top_LOW >> 0) & 0xFF; //Then get the last eight bits starting from left and do the same

  while(EECR & (1 << EEPE)); //Wait until the EEPE bit is 0, indicates that the last write operation is done
  EEAR = 1; //Write to the first EEPROM address
  EEDR = bytes[0]; //Write the first eight bits from the value, starting from left, to EEPROM
  EECR |= (1 << EEMPE); //EEPROM master write enable, clears automatically after 4 cycles, also bits EEPM1 EEPM0 are 0 so erase and write with same opeartion
  EECR |= (1 << EEPE); //Write 1 to EEPE to start writing
  while(EECR & (1 << EEPE));
  EEAR = 2;
  EEDR = bytes[1];
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE); 
  while(EECR & (1 << EEPE));

  //Top moisture sensor high threshold
  bytes[0] = (Limits.moisture_top_HIGH >> 8) & 0xFF;
  bytes[1] = (Limits.moisture_top_HIGH >> 0) & 0xFF;

  while(EECR & (1 << EEPE));
  EEAR = 3;
  EEDR = bytes[0];
  EECR |= (1 << EEMPE);
  EECR = (1 << EEPE);
  while(EECR & (1 << EEPE));
  EEAR = 4;
  EEDR = bytes[1];
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE); 
  while(EECR & (1 << EEPE));

  //Bottom moisture sensor high threshold
  bytes[0] = (Limits.moisture_bottom_HIGH >> 8) & 0xFF;
  bytes[1] = (Limits.moisture_bottom_HIGH >> 0) & 0xFF;

  while(EECR & (1 << EEPE));
  EEAR = 5;
  EEDR = bytes[0];
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE);
  while(EECR & (1 << EEPE));
  EEAR = 6;
  EEDR = bytes[1];
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE); 
  while(EECR & (1 << EEPE));

  //Write the remaining data's, which are all 1 byte each so no bitwise manipulation needed here since eeprom saves 1 byte at a time

  //Air temperature minimum threshold
  EEAR = 7;
  EEDR = Limits.temp_air_MIN;
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE);
  while(EECR & (1 << EEPE));


  //Water temperature minimum threshold
  EEAR = 8;
  EEDR = Limits.temp_water_MIN;
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE);
  while(EECR & (1 << EEPE));
  
  //Water level minimum threshold
  EEAR = 9;
  EEDR = Limits.water_level_LOW;
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE);
  while(EECR & (1 << EEPE));

  //The height of the water container
  EEAR = 10;
  EEDR = Limits.water_bucket_height;
  EECR |= (1 << EEMPE);
  EECR |= (1 << EEPE);
  while(EECR & (1 << EEPE));

  sei(); //Enable interrupts after saving is done
}

void read_from_eeprom(){
  
  cli();
  uint8_t bytes[2]; //Array to store the uint16_t values byte by byte read from eeprom
  while(EECR & (1 << EEPE)); //Wait if there's a write operation happening in eeprom
  EEAR = 1; //eeprom address
  EECR |= (1 << EERE); //Enable read from eeprom
  bytes[0] = EEDR; //The first eight bits for the complete data

  while(EECR & (1 << EEPE));
  EEAR = 2;
  EECR |= (1 << EERE);
  bytes[1] = EEDR;

  Limits.moisture_top_LOW = (bytes[0] << 8) | bytes[1]; //Add the two bytes to create the whole value

  while(EECR & (1 << EEPE));
  EEAR = 3;
  EECR |= (1 << EERE);
  bytes[0] = EEDR;

  while(EECR & (1 << EEPE));
  EEAR = 4;
  EECR |= (1 << EERE);
  bytes[1] = EEDR;

  Limits.moisture_top_HIGH = (bytes[0] << 8) | bytes[1];

  while(EECR & (1 << EEPE));
  EEAR = 5;
  EECR |= (1 << EERE);
  bytes[0] = EEDR;

  while(EECR & (1 << EEPE));
  EEAR = 6;
  EECR |= (1 << EERE);
  bytes[1] = EEDR;

  Limits.moisture_bottom_HIGH = (bytes[0] << 8) | bytes[1];

  //Then for the one byte datas
  while(EECR & (1 << EEPE));
  EEAR = 7;
  EECR |= (1 << EERE);
  Limits.temp_air_MIN = EEDR;

  while(EECR & (1 << EEPE));
  EEAR = 8;
  EECR |= (1 << EERE);
  Limits.temp_water_MIN = EEDR;

  while(EECR & (1 << EEPE));
  EEAR = 9;
  EECR |= (1 << EERE);
  Limits.water_level_LOW = EEDR;

  while(EECR & (1 << EEPE));
  EEAR = 10;
  EECR |= (1 << EERE);    
  Limits.water_bucket_height = EEDR;

  sei();
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