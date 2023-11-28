#include <Wire.h> // allows communication with I2C display
#include "rgb_lcd.h" // for controlling LCD-display
#include <math.h>
#include <string.h>
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

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);

  Timer1.initialize(1000000); // call every 1 second(s)
  Timer1.attachInterrupt(timerOneISR); // TimerOne interrupt callback

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
  Serial.println(Current_reading_digi.water_level_d);
}

void printout_formatter() {
  /*WIP*/
}