/**
* Прошивка SONOFF TH10/16
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/


#ifndef WS_HTTP_h
#define WS_HTTP_h
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>

extern ESP8266WebServer server;
extern bool isAP;
extern bool isConnect;
extern float Hum;
extern float Temp;
extern uint32_t first_tm;
extern uint32_t tm;
extern uint32_t t_cur;
extern bool ds_enable;
extern bool dht_enable;
extern bool relay_on;
extern uint8_t PIN_RELAY;


bool ConnectWiFi(void);
void HTTP_begin(void);
void HTTP_handleRoot(void);
void HTTP_handleConfig(void);
void HTTP_handleConfig2(void);
void HTTP_handleSave(void);
void HTTP_handleDefault(void);
void HTTP_handleReboot(void);
void HTTP_handleButton(void);
void HTTP_loop();
void WiFi_begin(void);
int  HTTP_isAuth();
void HTTP_handleLogin(void);
void HTTP_handleReboot(void);
void HTTP_gotoLogin();
int  HTTP_checkAuth(char *user);
void HTTP_printInput(String &out,const char *label, const char *name, const char *value, int size, int len,bool is_pass=false);
void HTTP_printHeader(String &out,const char *title, uint16_t refresh=0);
void HTTP_printTail(String &out);


#endif
