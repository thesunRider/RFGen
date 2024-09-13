#include <FastPwmPin.h>
#include <LiquidCrystal.h>
#include <Rotary.h>
#include <si5351mcu.h>
#include <EButton.h>

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

A0 - rots_clk
A1 - rots_dt
A2 - rots_sw

12 - rotm_clk
11 - rotm_dt
13 - rotm_sw
*/

#define BIAS_PIN 9
#define BUZZ_PIN A3
#define PWM_PIN2 3
#define PWM_PIN1 10

#define ROT_SELECT_CLK A0
#define ROT_SELECT_DT A1
#define ROT_SELECT_SW A2

#define ROT_MODE_CLK 12
#define ROT_MODE_DT 11
#define ROT_MODE_SW 13


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


// lib instantiation as "Si"
Si5351mcu Si;

Rotary rselect = Rotary(ROT_SELECT_CLK, ROT_SELECT_DT);
Rotary rmode = Rotary(ROT_MODE_CLK, ROT_MODE_DT);

EButton rot_select_button(ROT_SELECT_SW, true);
EButton rot_mode_button(ROT_MODE_SW, true);


unsigned long pwm1freq;
int pwm1duty;

unsigned long pwm2freq;
int pwm2duty;

#define SPLASH_DELAY 700
int buzz_duration = 30;

// ----- lcd declarations --------

#define LEFT_OPT 0
#define RIGHT_OPT 1
#define RIGHT_ARW 2
#define LEFT_ARW 3
#define BACK_MENU 4
#define BIAS_MENU 5

byte left_options[] = {
  B00000,
  B00001,
  B00011,
  B00111,
  B01111,
  B00111,
  B00011,
  B00001
};

byte right_options[] = {
  B00000,
  B10000,
  B11000,
  B11100,
  B11110,
  B11100,
  B11000,
  B10000
};

byte right_arrow[] = {
  B00000,
  B00100,
  B00010,
  B11111,
  B00010,
  B00100,
  B00000,
  B00000
};

byte left_arrow[] = {
  B00000,
  B00100,
  B01000,
  B11111,
  B01000,
  B00100,
  B00000,
  B00000
};

byte back_menu[] = {
  B00000,
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B11100,
  B00000
};

byte bias_char[] = {
  B00000,
  B10000,
  B10001,
  B10000,
  B11001,
  B10101,
  B11001,
  B00000
};

#define NUM_PAGES 6
#define PAGE_CHNA 0
#define PAGE_CHNB 1
#define PAGE_CHNC 2
#define PAGE_CHND 3
#define PAGE_SWEP 4
#define PAGE_PROG 5
#define PAGE_SET 6

#define SELECT_SW 0
#define MODE_SW 1

bool switch_sel_status = 0;
bool switch_mode_status = 0;

void formatfloat(float num, int totalWidth, int precision, char *buffer) {
  int integerPart = static_cast<int>(num);  // Extract integer part
  float decimalPart = num - integerPart;    // Extract decimal part


  // Multiply decimal part to convert it into an integer (e.g., 0.34 -> 34)
  for (int i = 0; i < precision; ++i) {
    decimalPart *= 10;
  }

  int decimalInt = static_cast<int>(decimalPart);  // Convert decimal to integer

  // Convert integer and decimal parts into a single string manually
  for (int i = 0; i < strlen(buffer); ++i) {
    buffer[i] = '0';
  }

  int numDigits = 1;
  if (integerPart != 0)
    numDigits = floor(log10(integerPart) + 1);


  // Calculate the number of leading zeros needed
  int leadingZeros = totalWidth - (numDigits + precision);

  // Add leading zeros
  for (int i = 0; i < leadingZeros; ++i) {
    buffer[i] = '0';
  }

  // Add integer part to buffer
  int index = leadingZeros;
  int tempInt = integerPart;
  if (tempInt != 0)
    for (int i = numDigits - 1; i >= 0; --i) {
      buffer[index + i] = '0' + (tempInt % 10);
      tempInt /= 10;
    }
  index += numDigits;

  // Add decimal point
  buffer[index++] = '.';

  // Add decimal part to buffer
  tempInt = decimalInt;
  if (decimalInt != 0)
    for (int i = precision - 1; i >= 0; --i) {
      buffer[index + i] = '0' + (tempInt % 10);
      tempInt /= 10;
    }

  buffer[index + precision] = '\0';  // Null-terminate the string
}

bool was_clicked(int switch_sel, bool consume) {
  int switch_temp_status;
  if (switch_sel == SELECT_SW) {
    switch_temp_status = switch_sel_status;
    if (consume)
      switch_sel_status = 0;
  } else {
    switch_temp_status = switch_mode_status;
    if (consume)
      switch_mode_status = 0;
  }
  return switch_temp_status;
}

void click_select(EButton &btn) {
  switch_sel_status = 1;
  digitalWrite(BUZZ_PIN, HIGH);
  delay(buzz_duration);
  digitalWrite(BUZZ_PIN, LOW);
}

void click_mode(EButton &btn) {
  switch_mode_status = 1;
  digitalWrite(BUZZ_PIN, HIGH);
  delay(buzz_duration);
  digitalWrite(BUZZ_PIN, LOW);
}


int options_page() {
  delay(100);
  lcd.clear();
  lcd.noCursor();
  lcd.setCursor(0, 0);
  lcd.write(byte(LEFT_OPT));
  lcd.setCursor(15, 0);
  lcd.write(byte(RIGHT_OPT));

  int option_selected = 0;

  lcd.setCursor(1, 0);
  lcd.print(F("  CHANNEL A"));
  lcd.setCursor(0, 1);
  lcd.print(F(" RF MW-VHF O/P"));

  rot_select_button.tick();
  rot_mode_button.tick();

  while (!was_clicked(SELECT_SW, true)) {
    // put your main code here, to run repeatedly:
    unsigned char rselect_event = rselect.process();
    unsigned char rmode_event = rmode.process();

    rot_select_button.tick();
    rot_mode_button.tick();

    //play sound
    if (rselect_event || rmode_event && buzz_duration > 0) {
      digitalWrite(BUZZ_PIN, HIGH);
      delay(buzz_duration);
      digitalWrite(BUZZ_PIN, LOW);
    }

    if (rselect_event) {
      rselect_event == DIR_CW ? option_selected++ : option_selected--;
      if (option_selected < 0)
        option_selected = NUM_PAGES;
      if (option_selected > NUM_PAGES)
        option_selected = 0;

      switch (option_selected) {
        case PAGE_CHNA:
          lcd.setCursor(1, 0);
          lcd.print(F("  CHANNEL A  "));
          lcd.setCursor(0, 1);
          lcd.print(F(" RF MW-VHF O/P"));
          break;

        case PAGE_CHNB:
          lcd.setCursor(1, 0);
          lcd.print(F("  CHANNEL B  "));
          lcd.setCursor(0, 1);
          lcd.print(F(" RF MW-VHF O/P"));
          break;

        case PAGE_CHNC:
          lcd.setCursor(1, 0);
          lcd.print(F("  CHANNEL C  "));
          lcd.setCursor(0, 1);
          lcd.print(F(" PWM 0-MW O/P "));
          break;

        case PAGE_CHND:
          lcd.setCursor(1, 0);
          lcd.print(F("  CHANNEL D  "));
          lcd.setCursor(0, 1);
          lcd.print(F(" PWM 0-MW O/P "));
          break;

        case PAGE_SWEP:
          lcd.setCursor(1, 0);
          lcd.print(F("  Sweep    "));
          lcd.setCursor(0, 1);
          lcd.print(F(" Freq Sweeper  "));
          break;

        case PAGE_PROG:
          lcd.setCursor(1, 0);
          lcd.print(F("  Prog mode  "));
          lcd.setCursor(0, 1);
          lcd.print(F(" Prog using i2c"));
          break;

        case PAGE_SET:
          lcd.setCursor(1, 0);
          lcd.print(F("  Options   "));
          lcd.setCursor(0, 1);
          lcd.print(F(" More Settings "));
          break;



        default:
          break;
      }
    }
  }
  switch_sel_status = 0;
  switch_mode_status = 0;

  Serial.println(option_selected);
  return option_selected;
}

#define OPTIONS_CHNA 5
#define CHN_LFFREQ 0
#define CHN_HFFREQ 1
#define CHN_ENABLE 2
#define CHN_PWR 5
#define CHN_BIAS 3
#define BACK_OPTION 4

int lffreq_chna = 1000;
float hffreq_chna = 1;
bool chnA_Enable = true;
int pwr_chna = 2;
bool bias_chnA = false;
unsigned long chna_freq = 1000;
void page_chnA() {
  lcd.clear();
  lcd.noCursor();
  lcd.setCursor(15, 1);
  lcd.write(byte(BACK_MENU));
  lcd.setCursor(13, 1);
  lcd.write(byte(BIAS_MENU));

  lcd.setCursor(0, 0);
  lcd.print(F("CHNA 1000hz 2mA"));
  lcd.setCursor(0, 1);
  lcd.print(F(" 001.00Mhz E"));

  lcd.setCursor(5, 0);
  char lfreq_char[5];

  sprintf(lfreq_char, "%04d", lffreq_chna);
  lcd.print(lfreq_char);

  lcd.setCursor(1, 1);
  char hfreq_char[7];
  dtostrf(hffreq_chna, 6, 2, hfreq_char);
  for (int i = 0; i < 7; i++)
    if (hfreq_char[i] == ' ')
      hfreq_char[i] = '0';  // Replace space with zero
  lcd.print(hfreq_char);

  lcd.setCursor(11, 1);
  chnA_Enable ? lcd.print("E") : lcd.print("D");

  lcd.setCursor(13, 1);
  bias_chnA ? lcd.print("B") : lcd.write(byte(BIAS_MENU));

  rot_select_button.tick();
  rot_mode_button.tick();
  int option_selected = -1;
  int selected_character = 0;

  lcd.setCursor(12, 0);
  lcd.print(pwr_chna);


  while (!(was_clicked(SELECT_SW, false) && option_selected == BACK_OPTION)) {
    // put your main code here, to run repeatedly:
    unsigned char rselect_event = rselect.process();
    unsigned char rmode_event = rmode.process();

    //play sound
    if (rselect_event || rmode_event && buzz_duration > 0) {
      digitalWrite(BUZZ_PIN, HIGH);
      delay(buzz_duration);
      digitalWrite(BUZZ_PIN, LOW);
    }

    if (was_clicked(MODE_SW, false) && option_selected == CHN_HFFREQ) {
      ++selected_character;
      if (selected_character == 3)
        ++selected_character;

      if (selected_character > 5)
        selected_character = 0;

      lcd.setCursor(1 + selected_character, 1);
      lcd.cursor();
    }

    if (was_clicked(SELECT_SW, false) && option_selected == CHN_ENABLE) {
      lcd.setCursor(11, 1);
      chnA_Enable = !chnA_Enable;
      chnA_Enable ? lcd.print("E") : lcd.print("D");
       chnA_Enable ? Si.enable(0) : Si.disable(0);
    }

    if (was_clicked(SELECT_SW, false) && option_selected == CHN_BIAS) {
      lcd.setCursor(13, 1);
      bias_chnA = !bias_chnA;
      bias_chnA ? lcd.print("B") : lcd.write(byte(BIAS_MENU));
      bias_chnA?digitalWrite(BIAS_PIN,HIGH):digitalWrite(BIAS_PIN,LOW);
    }

    if (rselect_event) {
      rselect_event == DIR_CW ? option_selected++ : option_selected--;
      if (option_selected < 0)
        option_selected = OPTIONS_CHNA;
      if (option_selected > OPTIONS_CHNA)
        option_selected = 0;

      lcd.noCursor();
      lcd.setCursor(4, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(10, 1);
      lcd.print(" ");
      lcd.setCursor(12, 1);
      lcd.print(" ");
      lcd.setCursor(11, 0);
      lcd.print(" ");

      switch (option_selected) {
        case CHN_LFFREQ:
          lcd.setCursor(4, 0);
          lcd.write(byte(RIGHT_ARW));
          break;

        case CHN_HFFREQ:
          lcd.setCursor(0, 1);
          lcd.write(byte(RIGHT_ARW));
          lcd.setCursor(1 + selected_character, 1);
          lcd.cursor();
          break;

        case CHN_ENABLE:
          lcd.setCursor(10, 1);
          lcd.write(byte(RIGHT_ARW));
          break;

        case CHN_PWR:
          lcd.setCursor(11, 0);
          lcd.write(byte(RIGHT_ARW));
          break;

        case CHN_BIAS:
          lcd.setCursor(12, 1);
          lcd.write(byte(RIGHT_ARW));
          break;

        case BACK_OPTION:
          lcd.setCursor(15, 1);
          lcd.cursor();
          break;

        default:
          break;
      }
    }

    if (rmode_event) {
      switch (option_selected) {
        case CHN_LFFREQ:
          lcd.setCursor(5, 0);
          rmode_event == DIR_CW ? lffreq_chna += 10 : lffreq_chna -= 10;
          if (lffreq_chna < 0)
            lffreq_chna = 0;
          if (lffreq_chna > 9999)
            lffreq_chna = 9999;
          sprintf(lfreq_char, "%04d", lffreq_chna);
          lcd.print(lfreq_char);

          if (chnA_Enable) {
            chna_freq = lffreq_chna + hffreq_chna;
            Si.setFreq(0, chna_freq);
            Si.enable(0);
          }
          break;

        case CHN_HFFREQ:
          lcd.setCursor(1, 1);
          if (selected_character <= 2)
            rmode_event == DIR_CW ? hffreq_chna += pow(10, (2 - selected_character)) : hffreq_chna -= pow(10, (2 - selected_character));
          else
            rmode_event == DIR_CW ? hffreq_chna += pow(10, (3 - selected_character)) : hffreq_chna -= pow(10, (3 - selected_character));
          if (hffreq_chna < 0)
            hffreq_chna = 0;
          if (hffreq_chna > 999.99)
            hffreq_chna = 999.99;

          formatfloat(hffreq_chna, 5, 2, hfreq_char);
          lcd.print(hfreq_char);
          lcd.setCursor(1 + selected_character, 1);
          lcd.cursor();

          if (chnA_Enable) {
            chna_freq = lffreq_chna + hffreq_chna;
            Si.setFreq(0, chna_freq);
            Si.enable(0);
          }
          break;

        case CHN_ENABLE:
          lcd.setCursor(10, 1);
          lcd.write(byte(RIGHT_ARW));
          break;

        case CHN_PWR:
          lcd.setCursor(12, 0);
          rmode_event == DIR_CW ? pwr_chna += 2 : pwr_chna -= 2;
          if (pwr_chna > 8)
            pwr_chna = 2;
          if (pwr_chna < 2)
            pwr_chna = 8;
          lcd.print(pwr_chna);
          if (pwr_chna == 2) {
            Si.setPower(0, SIOUT_2mA);
          } else if (pwr_chna == 4) {
            Si.setPower(0, SIOUT_4mA);
          } else if (pwr_chna == 6) {
            Si.setPower(0, SIOUT_6mA);
          } else if (pwr_chna == 8) {
            Si.setPower(0, SIOUT_8mA);
          }
          break;

        case CHN_BIAS:
          lcd.setCursor(12, 1);
          lcd.write(byte(RIGHT_ARW));
          break;

        case BACK_OPTION:
          lcd.setCursor(15, 1);
          lcd.cursor();
          break;

        default:
          break;
      }
    }


    switch_sel_status = 0;
    switch_mode_status = 0;
    rot_select_button.tick();
    rot_mode_button.tick();
  }
  switch_sel_status = 0;
  switch_mode_status = 0;
}


#define OPTIONS_CHNB 4
#define PWR_CHNB 3

int lffreq_chnb = 1000;
float hffreq_chnb = 1;
bool chnB_Enable = true;
int pwr_chnb = 2;
bool bias_chnB = false;
unsigned long chnb_freq = 1000;
void page_chnB() {
  lcd.clear();
  lcd.noCursor();
  lcd.setCursor(15, 1);
  lcd.write(byte(BACK_MENU));

  lcd.setCursor(0, 0);
  lcd.print(F("CHNB 1000hz 2mA"));
  lcd.setCursor(0, 1);
  lcd.print(F(" 001.00Mhz E"));

  lcd.setCursor(5, 0);
  char lfreq_char[5];
  sprintf(lfreq_char, "%04d", lffreq_chnb);
  lcd.print(lfreq_char);

  lcd.setCursor(1, 1);
  char hfreq_char[7];
  dtostrf(hffreq_chnb, 6, 2, hfreq_char);
  for (int i = 0; i < 7; i++)
    if (hfreq_char[i] == ' ')
      hfreq_char[i] = '0';  // Replace space with zero
  lcd.print(hfreq_char);

  lcd.setCursor(11, 1);
  chnB_Enable ? lcd.print("E") : lcd.print("D");


  rot_select_button.tick();
  rot_mode_button.tick();
  int option_selected = -1;
  int selected_character = 0;

  lcd.setCursor(12, 0);
  lcd.print(pwr_chnb);


  while (!(was_clicked(SELECT_SW, false) && option_selected == BACK_OPTION)) {
    // put your main code here, to run repeatedly:
    unsigned char rselect_event = rselect.process();
    unsigned char rmode_event = rmode.process();

    //play sound
    if (rselect_event || rmode_event && buzz_duration > 0) {
      digitalWrite(BUZZ_PIN, HIGH);
      delay(buzz_duration);
      digitalWrite(BUZZ_PIN, LOW);
    }

    if (was_clicked(MODE_SW, false) && option_selected == CHN_HFFREQ) {
      ++selected_character;
      if (selected_character == 3)
        ++selected_character;

      if (selected_character > 5)
        selected_character = 0;

      lcd.setCursor(1 + selected_character, 1);
      lcd.cursor();
    }

    if (was_clicked(SELECT_SW, false) && option_selected == CHN_ENABLE) {
      lcd.setCursor(11, 1);
      chnB_Enable = !chnB_Enable;
      chnB_Enable ? lcd.print("E") : lcd.print("D");
      chnB_Enable ? Si.enable(2) : Si.disable(2);
    }

    if (rselect_event) {
      rselect_event == DIR_CW ? option_selected++ : option_selected--;
      if (option_selected < 0)
        option_selected = OPTIONS_CHNB;
      if (option_selected > OPTIONS_CHNB)
        option_selected = 0;

      lcd.noCursor();
      lcd.setCursor(4, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(10, 1);
      lcd.print(" ");
      lcd.setCursor(12, 1);
      lcd.print(" ");
      lcd.setCursor(11, 0);
      lcd.print(" ");

      switch (option_selected) {
        case CHN_LFFREQ:
          lcd.setCursor(4, 0);
          lcd.write(byte(RIGHT_ARW));
          break;

        case CHN_HFFREQ:
          lcd.setCursor(0, 1);
          lcd.write(byte(RIGHT_ARW));
          lcd.setCursor(1 + selected_character, 1);
          lcd.cursor();
          break;

        case CHN_ENABLE:
          lcd.setCursor(10, 1);
          lcd.write(byte(RIGHT_ARW));
          break;

        case PWR_CHNB:
          lcd.setCursor(11, 0);
          lcd.write(byte(RIGHT_ARW));
          break;


        case BACK_OPTION:
          lcd.setCursor(15, 1);
          lcd.cursor();
          break;

        default:
          break;
      }
    }

    if (rmode_event) {
      switch (option_selected) {
        case CHN_LFFREQ:
          lcd.setCursor(5, 0);
          rmode_event == DIR_CW ? lffreq_chnb += 10 : lffreq_chnb -= 10;
          if (lffreq_chnb < 0)
            lffreq_chnb = 0;
          if (lffreq_chnb > 9999)
            lffreq_chnb = 9999;
          sprintf(lfreq_char, "%04d", lffreq_chnb);
          lcd.print(lfreq_char);
          if (chnB_Enable) {
            chnb_freq = lffreq_chnb + hffreq_chnb;
            Si.setFreq(2, chnb_freq);
            Si.enable(2);
          }
          break;

        case CHN_HFFREQ:
          lcd.setCursor(1, 1);
          if (selected_character <= 2)
            rmode_event == DIR_CW ? hffreq_chnb += pow(10, (2 - selected_character)) : hffreq_chnb -= pow(10, (2 - selected_character));
          else
            rmode_event == DIR_CW ? hffreq_chnb += pow(10, (3 - selected_character)) : hffreq_chnb -= pow(10, (3 - selected_character));
          if (hffreq_chnb < 0)
            hffreq_chnb = 0;
          if (hffreq_chnb > 999.99)
            hffreq_chnb = 999.99;

          formatfloat(hffreq_chnb, 5, 2, hfreq_char);
          lcd.print(hfreq_char);
          lcd.setCursor(1 + selected_character, 1);
          lcd.cursor();
          if (chnB_Enable) {
            chnb_freq = lffreq_chnb + hffreq_chnb;
            Si.setFreq(2, chnb_freq);
            Si.enable(2);
          }

          break;

        case CHN_ENABLE:
          lcd.setCursor(10, 1);
          lcd.write(byte(RIGHT_ARW));
          break;

        case PWR_CHNB:
          lcd.setCursor(12, 0);
          rmode_event == DIR_CW ? pwr_chnb += 2 : pwr_chnb -= 2;
          if (pwr_chnb > 8)
            pwr_chnb = 2;
          if (pwr_chnb < 2)
            pwr_chnb = 8;
          lcd.print(pwr_chnb);
          if (pwr_chnb == 2) {
            Si.setPower(2, SIOUT_2mA);
          } else if (pwr_chnb == 4) {
            Si.setPower(2, SIOUT_4mA);
          } else if (pwr_chnb == 6) {
            Si.setPower(2, SIOUT_6mA);
          } else if (pwr_chnb == 8) {
            Si.setPower(2, SIOUT_8mA);
          }

          break;



        case BACK_OPTION:
          lcd.setCursor(15, 1);
          lcd.cursor();
          break;

        default:
          break;
      }
    }


    switch_sel_status = 0;
    switch_mode_status = 0;
    rot_select_button.tick();
    rot_mode_button.tick();
  }
  switch_sel_status = 0;
  switch_mode_status = 0;
}

#define OPTIONS_CHNC 3
#define DUTY_C 0
#define FREQ_C 1
#define ENABLE_C 2
#define BACK_OPTION 3

int duty_c = 50;
float freq_c = 1;  //khz
unsigned long freq_apply_c = freq_c;
bool enable_c = false;
void page_chnC() {
  lcd.clear();
  lcd.noCursor();
  lcd.setCursor(15, 1);
  lcd.write(byte(BACK_MENU));
  lcd.setCursor(0, 0);
  lcd.print("PWM CHNC");
  lcd.setCursor(9, 0);
  lcd.print(duty_c);
  lcd.print("%");
  lcd.setCursor(9, 1);
  lcd.print("Khz");

  lcd.setCursor(14, 0);
  enable_c ? lcd.print("E") : lcd.print("D");

  if (enable_c) {
    freq_apply_c = 1000 * freq_c;
    FastPwmPin::enablePwmPin(PWM_PIN1, freq_apply_c, duty_c);
  } else {
    digitalWrite(PWM_PIN1, HIGH);
  }

  lcd.setCursor(1, 1);
  char hfreq_char[9];
  char duty_char[3];
  dtostrf(freq_c, 8, 3, hfreq_char);
  for (int i = 0; i < 9; i++)
    if (hfreq_char[i] == ' ')
      hfreq_char[i] = '0';  // Replace space with zero
  lcd.print(hfreq_char);

  rot_select_button.tick();
  rot_mode_button.tick();
  int option_selected = -1;
  int selected_character = 0;

  while (!(was_clicked(SELECT_SW, false) && option_selected == BACK_OPTION)) {
    // put your main code here, to run repeatedly:
    unsigned char rselect_event = rselect.process();
    unsigned char rmode_event = rmode.process();

    //play sound
    if (rselect_event || rmode_event && buzz_duration > 0) {
      digitalWrite(BUZZ_PIN, HIGH);
      delay(buzz_duration);
      digitalWrite(BUZZ_PIN, LOW);
    }

    if (was_clicked(SELECT_SW, false) && option_selected == ENABLE_C) {
      lcd.setCursor(14, 0);
      enable_c = !enable_c;
      enable_c ? lcd.print("E") : lcd.print("D");

      if (enable_c) {
        freq_apply_c = 1000 * freq_c;
        FastPwmPin::enablePwmPin(PWM_PIN1, freq_apply_c, duty_c);
      } else {
        digitalWrite(PWM_PIN1, HIGH);
      }
    }

    if (was_clicked(MODE_SW, false) && option_selected == FREQ_C) {
      ++selected_character;
      if (selected_character == 4)
        ++selected_character;

      if (selected_character > 7)
        selected_character = 0;

      lcd.setCursor(1 + selected_character, 1);
      lcd.cursor();
    }


    if (rselect_event) {
      rselect_event == DIR_CW ? option_selected++ : option_selected--;
      if (option_selected < 0)
        option_selected = OPTIONS_CHNC;
      if (option_selected > OPTIONS_CHNC)
        option_selected = 0;

      lcd.noCursor();
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(13, 0);
      lcd.print(" ");
      lcd.setCursor(8, 0);
      lcd.print(" ");

      switch (option_selected) {
        case BACK_OPTION:
          lcd.setCursor(15, 1);
          lcd.cursor();
          break;

        case FREQ_C:
          lcd.setCursor(0, 1);
          lcd.write(byte(RIGHT_ARW));
          lcd.setCursor(1 + selected_character, 1);
          lcd.cursor();
          break;

        case ENABLE_C:
          lcd.setCursor(13, 0);
          lcd.write(byte(RIGHT_ARW));
          break;

        case DUTY_C:
          lcd.setCursor(8, 0);
          lcd.write(byte(RIGHT_ARW));
          break;
      }
    }

    if (rmode_event) {
      switch (option_selected) {
        case DUTY_C:
          lcd.setCursor(9, 0);
          rmode_event == DIR_CW ? duty_c++ : duty_c--;
          if (duty_c < 2)
            duty_c = 98;
          if (duty_c > 98)
            duty_c = 0;
          sprintf(duty_char, "%02d", duty_c);
          lcd.print(duty_char);
          if (enable_c) {
            freq_apply_c = 1000 * freq_c;
            FastPwmPin::enablePwmPin(PWM_PIN1, freq_apply_c, duty_c);
          } else {
            digitalWrite(PWM_PIN1, HIGH);
          }
          break;

        case FREQ_C:
          lcd.setCursor(1, 1);
          if (selected_character <= 3)
            rmode_event == DIR_CW ? freq_c += pow(10, (3 - selected_character)) : freq_c -= pow(10, (3 - selected_character));
          else
            rmode_event == DIR_CW ? freq_c += pow(10, (4 - selected_character)) : freq_c -= pow(10, (4 - selected_character));
          if (freq_c < 0)
            freq_c = 0;
          if (freq_c > 9999.999)
            freq_c = 9999.999;

          dtostrf(freq_c, 7, 3, hfreq_char);
          formatfloat(freq_c, 7, 3, hfreq_char);

          lcd.print(hfreq_char);
          lcd.setCursor(1 + selected_character, 1);
          lcd.cursor();

          if (enable_c) {
            freq_apply_c = 1000 * freq_c;
            FastPwmPin::enablePwmPin(PWM_PIN1, freq_apply_c, duty_c);
          } else {
            digitalWrite(PWM_PIN1, HIGH);
          }

          break;
      }
    }


    switch_sel_status = 0;
    switch_mode_status = 0;
    rot_select_button.tick();
    rot_mode_button.tick();
  }
  switch_sel_status = 0;
  switch_mode_status = 0;
}



int duty_d = 50;
float freq_d = 1;  //khz
unsigned long freq_apply_d = freq_d;
bool enable_d = false;
void page_chnD() {
  lcd.clear();
  lcd.noCursor();
  lcd.setCursor(15, 1);
  lcd.write(byte(BACK_MENU));
  lcd.setCursor(0, 0);
  lcd.print("PWM CHND");
  lcd.setCursor(9, 0);
  lcd.print(duty_d);
  lcd.print("%");
  lcd.setCursor(9, 1);
  lcd.print("Khz");

  lcd.setCursor(14, 0);
  enable_d ? lcd.print("E") : lcd.print("D");

  if (enable_d) {
    freq_apply_d = 1000 * freq_d;
    FastPwmPin::enablePwmPin(PWM_PIN2, freq_apply_d, duty_d);
  } else {
    digitalWrite(PWM_PIN2, HIGH);
  }

  lcd.setCursor(1, 1);
  char hfreq_char[9];
  char duty_char[3];
  dtostrf(freq_d, 8, 3, hfreq_char);
  for (int i = 0; i < 9; i++)
    if (hfreq_char[i] == ' ')
      hfreq_char[i] = '0';  // Replace space with zero
  lcd.print(hfreq_char);

  rot_select_button.tick();
  rot_mode_button.tick();
  int option_selected = -1;
  int selected_character = 0;

  while (!(was_clicked(SELECT_SW, false) && option_selected == BACK_OPTION)) {
    // put your main code here, to run repeatedly:
    unsigned char rselect_event = rselect.process();
    unsigned char rmode_event = rmode.process();

    //play sound
    if (rselect_event || rmode_event && buzz_duration > 0) {
      digitalWrite(BUZZ_PIN, HIGH);
      delay(buzz_duration);
      digitalWrite(BUZZ_PIN, LOW);
    }

    if (was_clicked(SELECT_SW, false) && option_selected == ENABLE_C) {
      lcd.setCursor(14, 0);
      enable_d = !enable_d;
      enable_d ? lcd.print("E") : lcd.print("D");

      if (enable_d) {
        freq_apply_d = 1000 * freq_d;
        FastPwmPin::enablePwmPin(PWM_PIN2, freq_apply_d, duty_d);
      } else {
        digitalWrite(PWM_PIN2, HIGH);
      }
    }

    if (was_clicked(MODE_SW, false) && option_selected == FREQ_C) {
      ++selected_character;
      if (selected_character == 4)
        ++selected_character;

      if (selected_character > 7)
        selected_character = 0;

      lcd.setCursor(1 + selected_character, 1);
      lcd.cursor();
    }


    if (rselect_event) {
      rselect_event == DIR_CW ? option_selected++ : option_selected--;
      if (option_selected < 0)
        option_selected = OPTIONS_CHNC;
      if (option_selected > OPTIONS_CHNC)
        option_selected = 0;

      lcd.noCursor();
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(13, 0);
      lcd.print(" ");
      lcd.setCursor(8, 0);
      lcd.print(" ");

      switch (option_selected) {
        case BACK_OPTION:
          lcd.setCursor(15, 1);
          lcd.cursor();
          break;

        case FREQ_C:
          lcd.setCursor(0, 1);
          lcd.write(byte(RIGHT_ARW));
          lcd.setCursor(1 + selected_character, 1);
          lcd.cursor();
          break;

        case ENABLE_C:
          lcd.setCursor(13, 0);
          lcd.write(byte(RIGHT_ARW));
          break;

        case DUTY_C:
          lcd.setCursor(8, 0);
          lcd.write(byte(RIGHT_ARW));
          break;
      }
    }

    if (rmode_event) {
      switch (option_selected) {
        case DUTY_C:
          lcd.setCursor(9, 0);
          rmode_event == DIR_CW ? duty_d++ : duty_d--;
          if (duty_d < 2)
            duty_d = 98;
          if (duty_d > 98)
            duty_d = 0;
          sprintf(duty_char, "%02d", duty_d);
          lcd.print(duty_char);
          if (enable_d) {
            freq_apply_d = 1000 * freq_d;
            FastPwmPin::enablePwmPin(PWM_PIN2, freq_apply_d, duty_d);
          } else {
            digitalWrite(PWM_PIN2, HIGH);
          }
          break;

        case FREQ_C:
          lcd.setCursor(1, 1);
          if (selected_character <= 3)
            rmode_event == DIR_CW ? freq_d += pow(10, (3 - selected_character)) : freq_d -= pow(10, (3 - selected_character));
          else
            rmode_event == DIR_CW ? freq_d += pow(10, (4 - selected_character)) : freq_d -= pow(10, (4 - selected_character));
          if (freq_d < 0)
            freq_d = 0;
          if (freq_d > 9999.999)
            freq_d = 9999.999;

          dtostrf(freq_d, 7, 3, hfreq_char);
          formatfloat(freq_d, 7, 3, hfreq_char);

          lcd.print(hfreq_char);
          lcd.setCursor(1 + selected_character, 1);
          lcd.cursor();
          if (enable_d) {
            freq_apply_d = 1000 * freq_d;
            FastPwmPin::enablePwmPin(PWM_PIN2, freq_apply_d, duty_d);
          } else {
            digitalWrite(PWM_PIN2, HIGH);
          }
          break;
      }
    }


    switch_sel_status = 0;
    switch_mode_status = 0;
    rot_select_button.tick();
    rot_mode_button.tick();
  }
  switch_sel_status = 0;
  switch_mode_status = 0;
}


void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);
  // Print a message to the LCD.

  lcd.createChar(LEFT_OPT, left_options);
  lcd.createChar(RIGHT_OPT, right_options);
  lcd.createChar(RIGHT_ARW, right_arrow);
  lcd.createChar(LEFT_ARW, left_arrow);
  lcd.createChar(BACK_MENU, back_menu);
  lcd.createChar(BIAS_MENU, bias_char);


  lcd.setCursor(0, 0);
  lcd.print(F("  RF Generator "));
  lcd.setCursor(0, 1);
  lcd.print(F(" v1.15 <Surya>"));

  pinMode(PWM_PIN1, OUTPUT);
  pinMode(PWM_PIN2, OUTPUT);
  pinMode(BIAS_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);

  digitalWrite(BIAS_PIN, LOW);
  digitalWrite(BUZZ_PIN, HIGH);

  digitalWrite(PWM_PIN2, LOW);
  digitalWrite(PWM_PIN1, LOW);

  rselect.begin(true);
  rmode.begin(true);

  Si.init();
  Si.reset();
  rot_select_button.attachSingleClick(click_select);
  rot_mode_button.attachSingleClick(click_mode);


  delay(SPLASH_DELAY);
  digitalWrite(BUZZ_PIN, LOW);
  lcd.clear();



  // put your setup code here, to run once:
  //FastPwmPin::enablePwmPin(PWM_PIN1, pwm1freq, 50);
}

void loop() {

  lcd.clear();
  int sel_option = options_page();
  lcd.clear();
  switch (sel_option) {
    case PAGE_CHNA:
      page_chnA();
      break;

    case PAGE_CHNB:
      page_chnB();
      break;

    case PAGE_CHNC:
      page_chnC();
      break;

    case PAGE_CHND:
      page_chnD();
      break;

    default:
      break;
  }
}
