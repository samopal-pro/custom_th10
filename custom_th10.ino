/**
* Прошивка SONOFF TH10/16
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/

#include <arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// Мультиплатформенная библиотека
// https://github.com/PaulStoffregen/OneWire
#include <OneWire.h>
// DHT от Adafruit 
// https://github.com/adafruit/DHT-sensor-library
#include <DHT.h> 

//#include <Time.h>
#include <EEPROM.h>
//#include <arduino.h>

#include "WC_EEPROM.h"
#include "WC_HTTP.h"
#include "WC_NTP.h"
#include "sav_button.h" 

#define TIMEOUT_NTP 600000

uint8_t PIN_RELAY =   12;
uint8_t PIN_LED2  =   13;

uint8_t PIN_DHT   =   14;
uint8_t PIN_DS    =   14;

uint8_t PIN_BUTTON =  0;

DHT dht(PIN_DHT, AM2301); 
OneWire ds(PIN_DS);
SButton b1(PIN_BUTTON ,100,5000,0,0,0); 

#define ERROR_VALUE 2147483647

bool ds_enable  = false;
bool dht_enable = false;
bool relay_on   = false;

 
uint8_t  blink_loop  = 0;
uint8_t  modes_count = 0; 
uint8_t  blink_mode  = 0B00000101;


uint32_t ms0        = 0;
uint32_t ms1        = 0;
uint32_t ms2        = 0;
uint32_t ms3        = 0;
uint32_t ms4        = 0;
uint32_t ms5        = 0;
float Hum           = 0;
float Temp          = 0;
uint32_t first_tm   = 0;
uint32_t tm         = 0;
uint32_t t_cur      = 0;    
long  t_correct     = 0;
uint16_t err_count  = 0;


void setup() {
// Последовательный порт для отладки
   Serial.begin(115200);
   Serial.printf("\n\nFree memory %d\n",ESP.getFreeHeap());

   delay(500);
   long ret;
// Инициализация DS18B20
   ret = (long)GetDS18X20();
   Serial.printf("Init DS18B20 on %d ... ",PIN_DS);
   if(  ret != ERROR_VALUE ){
       ds_enable = true;
       Serial.println("ok");
   }
   else {
      Serial.println("fail");
   }

// Инициализация DHT
   Serial.printf("Init DHT21 on %d ... ",PIN_DHT);
   dht.begin(); 
   delay(500);
   ret = (long)dht.readTemperature();
//   Serial.printf(" %ld ",ret);
   if( ret != ERROR_VALUE ){
       dht_enable = true;
       Serial.println("ok");
   }
   else {
      Serial.println("fail");
   }

 
// Инициализация EEPROM
   EC_begin();  
   EC_read();
// Инициализация датчика DGT11   
// Подключаемся к WiFi  
  WiFi_begin();
  if( isConnect )blink_mode = 0B11111111;
  else blink_mode = 0B00000101;
  delay(2000);
// Старт внутреннего WEB-сервера  
  HTTP_begin(); 
  char str[20];  
  
// Инициализация UDP клиента для NTP
  NTP_begin();  
  pinMode(PIN_RELAY,OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  b1.begin();  

     
}


 




void loop() {
   uint32_t ms = millis();
   t_cur       = ms/1000;
   switch( b1.Loop() ){
      case  SB_CLICK :
         relay_on = !relay_on;
         digitalWrite(PIN_RELAY,relay_on);
         break;
      case  SB_LONG_CLICK :
         isAP = !isAP;
         if( isAP )blink_mode = 0B00010001;
         WiFi_begin();
         break;
   }
    

// Событие срабатывающее каждые 125 мс   
   if( ( ms - ms1 ) > 125|| ms < ms1 ){
       ms1 = ms;
// Режим светодиода ищем по битовой маске       
       if(  blink_mode & 1<<(blink_loop&0x07) ) digitalWrite(PIN_LED2, LOW); 
       else  digitalWrite(PIN_LED2, HIGH);
       blink_loop++;    
    }


   if(ms < ms2 || (ms - ms2) > 500 ){
      ms2 = ms;
      if( isAP == false ){
         if( WiFi.status() == WL_CONNECTED )blink_mode = 0B11111111;
         else blink_mode = 0B00000101;
      }
      tm    = t_cur + t_correct;
   }
      
   if(ms < ms3 || (ms - ms3) > 5000 ){
      ms3 = ms;
      if( isAP == false ){
         if( WiFi.status() != WL_CONNECTED )WiFi_begin();
      }
      if( ds_enable ){
         Temp = GetDS18X20();
         Serial.print("DS18B20 temp=");
         Serial.println(Temp,1);
      }
      if( dht_enable ){
         Temp = dht.readTemperature();
         Hum =  dht.readHumidity();
         Serial.print("AM2301 temp=");
         Serial.print(Temp,1);
         Serial.print(",hum=");
         Serial.println(Hum,0);
      }
      
      
   }
   

// Каждые 600 секунд считываем время в интернете 
   if( isConnect && !isAP && (ms < ms4 || (ms - ms4) > TIMEOUT_NTP || ms4 == 0 ) ){
       err_count++;
       ms4       = ms;
// Делаем три  попытки синхронизации с интернетом

       uint32_t t = GetNTP();
       if( t!=0 ){
//          ms2       = ms;
          if( first_tm == 0 )first_tm = t;
          err_count = 0;
          t_correct = t - t_cur;
       }
       Serial.printf("Get NTP time. Error = %d\n",err_count);
   }
    
// Отправка на сервер
/*
   if(ms < ms5 || (ms - ms5) > EC_Config.TIMEOUT_SEND1 || 
      (ms - ms5) > EC_Config.TIMEOUT_SEND2 && (flag_light || flag_cool) ){
      Serial.printf("%ld %ld %ld %ld %d %d\n",ms,ms5,EC_Config.TIMEOUT_SEND1,EC_Config.TIMEOUT_SEND2,
         (int)flag_light,(int)flag_cool);  
      ms5 = ms;
      if( SetParamHTTP() ){
         Count1 = 0;
         Count2 = 0;
         err_count = 0;
      }
      else {
         err_count++;
      }

   }

   if( err_count>=5 && !flag_cool )ESP.reset();
*/
   HTTP_loop();

}


/**
* Опрос датчика температуры
*/

float  GetDS18X20(){
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  
  if ( !ds.search(addr)) {
//    Serial.println("DS18B20: No more addresses.");
    ds.reset_search();
    delay(250);
    return ERROR_VALUE;
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) {
//      Serial.println("DS1820: CRC is not valid!");
      return ERROR_VALUE;
  }
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
       type_s = 0;
      break;
    default:
//      Serial.println("Device is not a DS18x20 family device.");
      return ERROR_VALUE;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  return celsius;
} 
