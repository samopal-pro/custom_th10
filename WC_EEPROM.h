/**
* Прошивка SONOFF TH10/16
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/


#ifndef WC_EEPROM_h
#define WC_EEPROM_h
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <EEPROM.h>

extern struct WC_Config EC_Config;
extern uint32_t tm;
extern bool flag_light;
extern bool flag_hum;
extern bool flag_btn;
extern uint16_t timer;

struct WC_Config{
// Наименование в режиме точки доступа  
   char ESP_NAME[32];
   char ESP_PASS[32];
// Параметры подключения в режиме клиента
   char AP_SSID[32];
   char AP_PASS[32];
// Пароль доступа к контроллеру
   char HTTP_PASS[32];
   
   IPAddress IP;
   IPAddress MASK;
   IPAddress GW;
// Параметры NTP сервера
   char NTP_SERVER1[32];
   char NTP_SERVER2[32];
   char NTP_SERVER3[32];
   short int  TZ;

// Сервер куда отправлять статистику
   char     HTTP_SERVER[48];   
// Строка отправки параметров на сервер
//   char     HTTP_REQUEST[128];   
// Контрольная сумма   
   uint16_t SRC;   
};


void     EC_begin(void);
void     EC_read(void);
void     EC_save(void);
uint16_t EC_SRC(void);
void     EC_default(void);





#endif
