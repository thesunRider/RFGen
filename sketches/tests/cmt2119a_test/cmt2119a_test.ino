/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: 
 *  (1) "AS IS" WITH NO WARRANTY; 
 *  (2) TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, HopeRF SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 * 
 * Copyright (C) HopeRF
 * website: www.HopeRF.com
 *          www.HopeRF.cn
 *
 */

/*! 
 * file       rfm119_Tx.ino
 * hardware   HopeDuino 
 * software   send message via rfm119
 * note       
 *
 * version    1.0
 * date       Feb 19 2016
 * author     QY Ruan
 */

#include "src/RFM119/HopeDuino_CMT211xA.h"

cmt211xaClass radio;

byte str[31] = {
  0xAA,
  0xAA,
  0xAA,
  0xAA,
  0xAA,
  0xAA,
  0xAA,
  0xAA,
  0x2D,
  0xD4,
  'H',
  'o',
  'p',
  'e',
  'R',
  'F',
  ' ',
  'R',
  'F',
  'M',
  ' ',
  'C',
  'O',
  'B',
  'R',
  'F',
  'M',
  '1',
  '1',
  '9',
  'S',
};

word CfgTbl[21] = {
0x007F,
0x5000,
0x0000,
0x0000,
0x0000,
0xF000,
0x0000,
0xBB13,
0x4200,
0x0000,
0x2401,
0x01B0,
0x8000,
0x0006,
0xFFFF,
0x0020,
0x5F1E,
0x22D6,
0x0E13,
0x0019,
0x2000,
};

void setup() {
  Serial.begin(9600);
  radio.Chipset = CMT2119A;
  radio.SymbolTime = 416;
  radio.vCMT2119AInit(CfgTbl, 21, 245000000UL, 10000UL, 0);
  //radio.vEncode(str, 31, ENRZ);
  radio.turnOnRadio();
}

void loop() {
  
  
  


}
