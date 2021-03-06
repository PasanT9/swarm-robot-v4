/**
* @brief    Driver for Adafruit's TCS34725 RGB Color Sensor Breakout
* @author   Adafruit Industries
* @version  1.3.1
* @url      https://github.com/adafruit/Adafruit_TCS34725
*
* Software License Agreement (BSD License)
* Copyright (c) 2013, Adafruit Industries
* ------------------------------------------------------------------------------
*/

#ifdef __AVR
#include <avr/pgmspace.h>
#elif defined(ESP8266)
#include <pgmspace.h>
#endif

#include <stdlib.h>
#include <math.h>

#include "../features.h"
#include "../src/pins.h"
#include "../src/SW_TCS34725.h"

#ifdef ENABLE_COLOR_SENSOR


//Implements missing powf function
float powf(const float x, const float y){
   return (float)(pow((double)x, (double)y));
}

//Writes a register and an 8 bit value over I2C
void SW_TCS34725::write8 (uint8_t reg, uint32_t value){
   Wire.beginTransmission(TCS34725_ADDRESS);
   #if ARDUINO >= 100
   Wire.write(TCS34725_COMMAND_BIT | reg);
   Wire.write(value & 0xFF);
   #else
   Wire.send(TCS34725_COMMAND_BIT | reg);
   Wire.send(value & 0xFF);
   #endif
   Wire.endTransmission();
}

//brief  Reads an 8 bit value over I2C
uint8_t SW_TCS34725::read8(uint8_t reg){
   Wire.beginTransmission(TCS34725_ADDRESS);
   #if ARDUINO >= 100
   Wire.write(TCS34725_COMMAND_BIT | reg);
   #else
   Wire.send(TCS34725_COMMAND_BIT | reg);
   #endif
   Wire.endTransmission();

   Wire.requestFrom(TCS34725_ADDRESS, 1);
   #if ARDUINO >= 100
   return Wire.read();
   #else
   return Wire.receive();
   #endif
}

//Reads a 16 bit values over I2C
uint16_t SW_TCS34725::read16(uint8_t reg){
   uint16_t x; uint16_t t;

   Wire.beginTransmission(TCS34725_ADDRESS);
   #if ARDUINO >= 100
   Wire.write(TCS34725_COMMAND_BIT | reg);
   #else
   Wire.send(TCS34725_COMMAND_BIT | reg);
   #endif
   Wire.endTransmission();

   Wire.requestFrom(TCS34725_ADDRESS, 2);
   #if ARDUINO >= 100
   t = Wire.read();
   x = Wire.read();
   #else
   t = Wire.receive();
   x = Wire.receive();
   #endif
   x <<= 8;
   x |= t;
   return x;
}

//Enables the device
void SW_TCS34725::enable(void){
   write8(TCS34725_ENABLE, TCS34725_ENABLE_PON);
   delay(3);
   write8(TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN);
   Serial.println(F(">> ColorSensor\t:enabled"));
}

//Disables the device (putting it in lower power sleep mode)
void SW_TCS34725::disable(void){
   /* Turn the device off to save power */
   uint8_t reg = 0;
   reg = read8(TCS34725_ENABLE);
   write8(TCS34725_ENABLE, reg & ~(TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN));
   Serial.println(F(">> ColorSensor\t:disabled"));
}

//Constructor
SW_TCS34725::SW_TCS34725(tcs34725IntegrationTime_t it, tcs34725Gain_t gain){
   _tcs34725Initialised = false;
   _tcs34725IntegrationTime = it;
   _tcs34725Gain = gain;
}


//Initializes I2C and configures the sensor (call this function before doing anything else)
boolean SW_TCS34725::begin(void){
   Wire.begin();

   /* Make sure we're actually connected */
   uint8_t x = read8(TCS34725_ID);
   //Serial.println(x, HEX);
   if (x != 0x44)
   {
      return false;
   }
   _tcs34725Initialised = true;

   /* Set default integration time and gain */
   setIntegrationTime(_tcs34725IntegrationTime);
   setGain(_tcs34725Gain);

   /* Note: by default, the device is in power down mode on bootup */
   enable();

   setInterrupt(true);
   generateGammaTable();

   return true;
}

void SW_TCS34725::generateGammaTable() {
   // it helps convert RGB colors to what humans see
   for (int i = 0; i < 256; i++) {
      float x = i;
      x /= 255;
      x = pow(x, 2.5);
      x *= 255;
      gammatable[i] = x;
      //Serial.println(gammatable[i]);
   }
}

//Sets the integration time for the TC34725
void SW_TCS34725::setIntegrationTime(tcs34725IntegrationTime_t it){
   if (!_tcs34725Initialised) begin();

   /* Update the timing register */
   write8(TCS34725_ATIME, it);

   /* Update value placeholders */
   _tcs34725IntegrationTime = it;
}

//Adjusts the gain on the TCS34725 (adjusts the sensitivity to light)
void SW_TCS34725::setGain(tcs34725Gain_t gain){
   if (!_tcs34725Initialised) begin();

   /* Update the timing register */
   write8(TCS34725_CONTROL, gain);

   /* Update value placeholders */
   _tcs34725Gain = gain;
}

//Reads the raw red, green, blue and clear channel values
void SW_TCS34725::getRawData (uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c){
   if (!_tcs34725Initialised) begin();

   *c = read16(TCS34725_CDATAL);
   *r = read16(TCS34725_RDATAL);
   *g = read16(TCS34725_GDATAL);
   *b = read16(TCS34725_BDATAL);

   /* Set a delay for the integration time */
   switch (_tcs34725IntegrationTime)
   {
      case TCS34725_INTEGRATIONTIME_2_4MS:
      delay(3);
      break;
      case TCS34725_INTEGRATIONTIME_24MS:
      delay(24);
      break;
      case TCS34725_INTEGRATIONTIME_50MS:
      delay(50);
      break;
      case TCS34725_INTEGRATIONTIME_101MS:
      delay(101);
      break;
      case TCS34725_INTEGRATIONTIME_154MS:
      delay(154);
      break;
      case TCS34725_INTEGRATIONTIME_700MS:
      delay(700);
      break;
   }
}

//Converts the raw R/G/B values to color temperature in degrees
uint16_t SW_TCS34725::calculateColorTemperature(uint16_t r, uint16_t g, uint16_t b){
   float X, Y, Z;      /* RGB to XYZ correlation      */
   float xc, yc;       /* Chromaticity co-ordinates   */
   float n;            /* McCamy's formula            */
   float cct;

   /* 1. Map RGB values to their XYZ counterparts.    */
   /* Based on 6500K fluorescent, 3000K fluorescent   */
   /* and 60W incandescent values for a wide range.   */
   /* Note: Y = Illuminance or lux                    */
   X = (-0.14282F * r) + (1.54924F * g) + (-0.95641F * b);
   Y = (-0.32466F * r) + (1.57837F * g) + (-0.73191F * b);
   Z = (-0.68202F * r) + (0.77073F * g) + ( 0.56332F * b);

   /* 2. Calculate the chromaticity co-ordinates      */
   xc = (X) / (X + Y + Z);
   yc = (Y) / (X + Y + Z);

   /* 3. Use McCamy's formula to determine the CCT    */
   n = (xc - 0.3320F) / (0.1858F - yc);

   /* Calculate the final CCT */
   cct = (449.0F * powf(n, 3)) + (3525.0F * powf(n, 2)) + (6823.3F * n) + 5520.33F;

   /* Return the results in degrees Kelvin */
   return (uint16_t)cct;
}

//Converts the raw R/G/B values to color temperature in degrees Kelvin
uint16_t SW_TCS34725::calculateLux(uint16_t r, uint16_t g, uint16_t b){
   float illuminance;

   /* This only uses RGB ... how can we integrate clear or calculate lux */
   /* based exclusively on clear since this might be more reliable?      */
   illuminance = (-0.32466F * r) + (1.57837F * g) + (-0.73191F * b);

   return (uint16_t)illuminance;
}

void SW_TCS34725::setInterrupt(boolean i) {
   uint8_t r = read8(TCS34725_ENABLE);
   if (i) {
      r |= TCS34725_ENABLE_AIEN;
   } else {
      r &= ~TCS34725_ENABLE_AIEN;
   }
   write8(TCS34725_ENABLE, r);
}
void SW_TCS34725::clearInterrupt(void) {
   Wire.beginTransmission(TCS34725_ADDRESS);
   #if ARDUINO >= 100
   Wire.write(0x66);
   #else
   Wire.send(0x66);
   #endif
   Wire.endTransmission();
}
void SW_TCS34725::setIntLimits(uint16_t low, uint16_t high) {
   write8(0x04, low & 0xFF);
   write8(0x05, low >> 8);
   write8(0x06, high & 0xFF);
   write8(0x07, high >> 8);
}

void SW_TCS34725::test(){
   uint16_t red, green, blue, c,  colorTemp, lux;
   float r, g, b;

   this->setInterrupt(false);      // turn on LED
   delay(60);

   this->getRawData(&red, &green, &blue, &c);
   colorTemp = this->calculateColorTemperature(red, green, blue);
   lux = this->calculateLux(red, green, blue);

   this->setInterrupt(true);  // turn off LED

   r = (int)((red * 256) / (float)c);
   g = (int)((green * 256) / (float)c);
   b = (int)((blue * 256) / (float)c);

   Serial.printf("Color Sensor:\n\tR:%d G:%d B:%d temp:%d lux:%d\n\n", (int)r, (int)g, (int)b, colorTemp, lux);
}
#else

SW_TCS34725::SW_TCS34725(tcs34725IntegrationTime_t it, tcs34725Gain_t gain){}

boolean SW_TCS34725::begin(void){
   Serial.println(F(">> ColorSensor\t:disabled"));
   return false;
}
void SW_TCS34725::generateGammaTable() {}
void SW_TCS34725::setIntegrationTime(tcs34725IntegrationTime_t it){}
void SW_TCS34725::setGain(tcs34725Gain_t gain){}
void SW_TCS34725::getRawData (uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c){}
uint16_t SW_TCS34725::calculateColorTemperature(uint16_t r, uint16_t g, uint16_t b){return 0;}
uint16_t SW_TCS34725::calculateLux(uint16_t r, uint16_t g, uint16_t b){return 0;}
void SW_TCS34725::setInterrupt(boolean i) {}
void SW_TCS34725::clearInterrupt(void){}
void SW_TCS34725::setIntLimits(uint16_t low, uint16_t high){}
void SW_TCS34725::test(){
   Serial.println(F(">> ColorSensor\t:disabled"));
}
#endif
