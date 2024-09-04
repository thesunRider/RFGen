#include <FastPwmPin.h>
#include <LiquidCrystal.h>
#include <Rotary.h>
#include <si5351mcu.h>


/*
1   2   3   4   5
RX  RST TX SCL SDA

atmega

RS - 8
Enable - 7
d7 - 2
d6 - 4
d5 - 5
d4 - 6

9 - bias
A3 - buzz


3 - pwm1
10 - pwm2

A2,A1,A0
11,12,13
*/

#define BIAS_PIN 9
#define BUZZ_PIN A3
#define PWM_PIN1 3
#define PWM_PIN2 10

#define ROT_SELECT_CLK 11
#define ROT_SELECT_DT 12
#define ROT_SELECT_SW 13

#define ROT_MODE_CLK A0
#define ROT_MODE_DT A1
#define ROT_MODE_SW A2

// Pin mapping for the display:
const byte LCD_RS = 8;
const byte LCD_E = 7;
const byte LCD_D4 = 6;
const byte LCD_D5 = 5;
const byte LCD_D6 = 4;
const byte LCD_D7 = 2;
//LCD R/W pin to ground
//10K potentiometer wiper to VO
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);


unsigned long pwm1freq;
int pwm1duty;

unsigned long pwm2freq;
int pwm2duty;

Rotary rselect = Rotary(2, 3);
Rotary rmode = Rotary(2, 3);

// lib instantiation as "Si"
Si5351mcu Si;

void setup() {
  pinMode(PWM_PIN1,OUTPUT);
  pinMode(PWM_PIN2,OUTPUT);

  digitalWrite(PWM_PIN2,LOW);
  digitalWrite(PWM_PIN1,LOW);

  rselect.begin(true);
  rmode.begin(true);

  Si.init();
  // put your setup code here, to run once:
  //FastPwmPin::enablePwmPin(PWM_PIN1, pwm1freq, 50);
}

void loop() {
  // put your main code here, to run repeatedly:

}
