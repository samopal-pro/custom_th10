/**
* Прошивка SONOFF TH10/16
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/


#include "WC_HTTP.h"
#include "WC_EEPROM.h"
#include "WC_NTP.h"

ESP8266WebServer server(80);
bool isAP       = false;
bool isConnect  = false;
String authPass = "";
String HTTP_User = "";
int    UID       = -1;

/**
 * Старт WiFi
 */
void WiFi_begin(void){ 
// Подключаемся к WiFi
  if( isAP  ){
      Serial.printf("Start AP %s\n",EC_Config.ESP_NAME);
      WiFi.mode(WIFI_AP);
      WiFi.softAP(EC_Config.ESP_NAME);
      Serial.printf("Open http://192.168.4.1 in your browser\n");
  }
  else {
// Получаем статический IP если нужно  
      WiFi.mode(WIFI_STA);
      isConnect = ConnectWiFi(); 
      if( isConnect && EC_Config.IP != 0 ){
         WiFi.config(EC_Config.IP,EC_Config.GW,EC_Config.MASK);
         Serial.print("Open http://");
         Serial.print(WiFi.localIP());
         Serial.println(" in your browser");
      }
   }
// Запускаем MDNS
//    MDNS.begin(EC_Config.ESP_NAME);
//    Serial.printf("Or by name: http://%s.local\n",EC_Config.ESP_NAME);
    

   
}

/**
 * Соединение с WiFi
 */
bool ConnectWiFi(void) {

  // Если WiFi не сконфигурирован
  if ( strcmp(EC_Config.AP_SSID, "none")==0 ) {
     Serial.printf("WiFi is not config ...\n");
     return false;
  }

  WiFi.mode(WIFI_STA);

  // Пытаемся соединиться с точкой доступа
  Serial.printf("\nConnecting to: %s/%s\n", EC_Config.AP_SSID, EC_Config.AP_PASS);
  WiFi.begin(EC_Config.AP_SSID, EC_Config.AP_PASS);
  delay(1000);

  // Максиммум N раз проверка соединения (12 секунд)
  for ( int j = 0; j < 15; j++ ) {
  if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\nWiFi connect: ");
      Serial.print(WiFi.localIP());
      Serial.print("/");
      Serial.print(WiFi.subnetMask());
      Serial.print("/");
      Serial.println(WiFi.gatewayIP());
      return true;
    }
    delay(1000);
    Serial.print(WiFi.status());
  }
  Serial.printf("\nConnect WiFi failed ...\n");
  return false;
}

/**
 * Старт WEB сервера
 */
void HTTP_begin(void){
// Поднимаем WEB-сервер  
   server.on ( "/", HTTP_handleRoot );
   server.on ( "/config", HTTP_handleConfig );
   server.on ( "/login", HTTP_handleLogin );
   server.on ( "/reboot", HTTP_handleReboot );
   server.onNotFound ( HTTP_handleRoot );
  //here the list of headers to be recorded
   const char * headerkeys[] = {"User-Agent","Cookie"} ;
   size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  //ask server to track these headers
   server.collectHeaders(headerkeys, headerkeyssize );


   
   server.begin();
   Serial.printf( "HTTP server started ...\n" );
  
}

/**
 * Обработчик событий WEB-сервера
 */
void HTTP_loop(void){
  server.handleClient();
}


/**
 * Вывод в буфер одного поля формы
 */
void HTTP_printInput(String &out,const char *label, const char *name, const char *value, int size, int len, bool is_pass){
   char str[10];
   if( strlen( label ) > 0 ){
      out += "<td>";
      out += label;
      out += "</td>\n";
   }
   out += "<td><input name ='";
   out += name;
   out += "' value='";
   out += value;
   out += "' size=";
   sprintf(str,"%d",size);  
   out += str;
   out += " length=";    
   sprintf(str,"%d",len);  
   out += str;
   if( is_pass )out += " type='password'";
   out += "></td>\n";  
}

/**
 * Вывод заголовка файла HTML
 */
void HTTP_printHeader(String &out,const char *title, uint16_t refresh){
  out += "<html>\n<head>\n<meta charset=\"utf-8\" />\n";
  if( refresh ){
     char str[10];
     sprintf(str,"%d",refresh);
     out += "<meta http-equiv='refresh' content='";
     out +=str;
     out +="'/>\n"; 
  }
  out += "<title>";
  out += title;
  out += "</title>\n";
  out += "<style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>\n</head>\n";
  out += "<body>\n";
  out += "<p><b>Устройство: ";
  out += EC_Config.ESP_NAME;
  if( UID < 0 )out +=" <a href=\"/login\">Авторизация</a>\n";
  else out +=" <a href=\"/login?DISCONNECT=YES\">Выход</a>\n";
  out += "</b></p>";
  out += title;
  out += "</h1>\n";
}   
 
/**
 * Выаод окончания файла HTML
 */
void HTTP_printTail(String &out){
  out += "</body>\n</html>\n";
}

/**
 * Ввод имени и пароля
 */
void HTTP_handleLogin(){
  String msg;
// Считываем куки  
  if (server.hasHeader("Cookie")){   
//    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
//    Serial.println(cookie);
  }
  if (server.hasArg("DISCONNECT")){
    Serial.println("Disconnect");
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESP_PASS=\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }
  if ( server.hasArg("PASSWORD") ){
    String pass = server.arg("PASSWORD");
    
    if ( HTTP_checkAuth((char *)pass.c_str()) >=0 ){
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESP_PASS="+pass+"\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      Serial.println("Login Success");
      return;
    }
  msg = "Неправильный пароль";
  Serial.println("Login Failed");
  }
  String out = "";
  HTTP_printHeader(out,"Авторизация");
  out += "<form action='/login' method='POST'>\
    <table border=0 width='600'>\
      <tr>\
        <td width='200'>Введите пароль:</td>\
        <td width='400'><input type='password' name='PASSWORD' placeholder='password' size='32' length='32'></td>\
      </tr>\
      <tr>\
        <td width='200'><input type='submit' name='SUBMIT' value='Ввод'></td>\
        <td width='400'>&nbsp</td>\
      </tr>\
    </table>\
    </form><b>" +msg +"</b><br>";
  HTTP_printTail(out);  
  server.send(200, "text/html", out);
}

/**
 * Перейти на страничку с авторизацией
 */
void HTTP_gotoLogin(){
  String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
  server.sendContent(header);
}


/*
 * Оработчик главной страницы сервера
 */
void HTTP_handleRoot(void) {
  char str[10];
// Проверка авторизации  

  int gid = HTTP_isAuth();
  if ( gid < 0 ){
    HTTP_gotoLogin();
    return;
  }  

// Сохранение контроллера
   if ( server.hasArg("POWER") ){
       relay_on = !relay_on;
       digitalWrite(PIN_RELAY,relay_on);
       String header = "HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
       server.sendContent(header);
       return;
   }
   String out = "";
   HTTP_printHeader(out,"Показания контроллера");

  
   out += "<h1>WiFi контроллер</h1>\n";
   out +="<h2>";
   out += EC_Config.ESP_NAME;
   out +="</h2>\n";
   
   if( isAP ){
      out += "<p>Режим точки доступа: ";
      out += EC_Config.ESP_NAME;
      out+= " </p>";
   }
   else {
      out += "<p>Подключено к ";
      out += EC_Config.AP_SSID;
      out += " </p>";
   }   
/*
 * Здесь выводим что-нибудь свое
 * 
 */
   out +="<h2>";
   sprintf(str,"Время: %02d:%02d</br>",( tm/3600 )%24,( tm/60 )%60);
   out +=str;
   if( ds_enable ){
      sprintf(str,"T=%02d<br>",(int)Temp);
      out +=str;
   }
   if( dht_enable ){
      sprintf(str,"T=%02d H=%0d <br>",(int)Temp,(int)Hum);
      out +=str;
   }
   out +="Реле: ";
   if( relay_on )out +="включено";
   else out +="выключено";

   out +="<form action='/' method='POST'><input type='submit' name='POWER' value='";
   if( relay_on )out +="ОТКЛЮЧИТЬ";
   else out+="ВКЛЮЧИТЬ";
   out+="'></form>";
  
  
   out+= "\
     <p><a href=\"/config\">Настройка параметров сети</a>\
  </body>\
</html>";
   server.send ( 200, "text/html", out );
}


  
 
/*
 * Оработчик страницы настройки сервера
 */
void HTTP_handleConfig(void) {
// Проверка прав администратора  
  char s[65];
  if ( HTTP_isAuth() != 0 ){
    HTTP_gotoLogin();
    return;
  } 

// Сохранение контроллера
  if ( server.hasArg("ESP_NAME") ){

     if( server.hasArg("ESP_NAME")     )strcpy(EC_Config.ESP_NAME,server.arg("ESP_NAME").c_str());
     if( server.hasArg("ESP_PASS")     )strcpy(EC_Config.ESP_PASS,server.arg("ESP_PASS").c_str());
     if( server.hasArg("AP_SSID")      )strcpy(EC_Config.AP_SSID,server.arg("AP_SSID").c_str());
     if( server.hasArg("AP_PASS")      )strcpy(EC_Config.AP_PASS,server.arg("AP_PASS").c_str());
     if( server.hasArg("IP1")          )EC_Config.IP[0] = atoi(server.arg("IP1").c_str());
     if( server.hasArg("IP2")          )EC_Config.IP[1] = atoi(server.arg("IP2").c_str());
     if( server.hasArg("IP3")          )EC_Config.IP[2] = atoi(server.arg("IP3").c_str());
     if( server.hasArg("IP4")          )EC_Config.IP[3] = atoi(server.arg("IP4").c_str());
     if( server.hasArg("MASK1")        )EC_Config.MASK[0] = atoi(server.arg("MASK1").c_str());
     if( server.hasArg("MASK2")        )EC_Config.MASK[1] = atoi(server.arg("MASK2").c_str());
     if( server.hasArg("MASK3")        )EC_Config.MASK[2] = atoi(server.arg("MASK3").c_str());
     if( server.hasArg("NASK4")        )EC_Config.MASK[3] = atoi(server.arg("MASK4").c_str());
     if( server.hasArg("GW1")          )EC_Config.GW[0] = atoi(server.arg("GW1").c_str());
     if( server.hasArg("GW2")          )EC_Config.GW[1] = atoi(server.arg("GW2").c_str());
     if( server.hasArg("GW3")          )EC_Config.GW[2] = atoi(server.arg("GW3").c_str());
     if( server.hasArg("GW4")          )EC_Config.GW[3] = atoi(server.arg("GW4").c_str());
     if( server.hasArg("HTTP_PASS")   ){
         if( strcmp(server.arg("HTTP_PASS").c_str(),"*") != 0 ){
             strcpy(EC_Config.HTTP_PASS,server.arg("WEB_PASS").c_str());
         }
     }
     if( server.hasArg("ntp_server1")  )strcpy(EC_Config.NTP_SERVER1,server.arg("ntp_server1").c_str());
     if( server.hasArg("ntp_server2")  )strcpy(EC_Config.NTP_SERVER2,server.arg("ntp_server2").c_str());
     if( server.hasArg("ntp_server3")  )strcpy(EC_Config.NTP_SERVER3,server.arg("ntp_server3").c_str());
     if( server.hasArg("tz")           )EC_Config.TZ = atoi(server.arg("tz").c_str());
     if( server.hasArg("http_serv")  )strcpy(EC_Config.HTTP_SERVER,server.arg("http_serv").c_str());
     EC_save();
     
     String header = "HTTP/1.1 301 OK\r\nLocation: /config\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
  }

  String out = "";
  char str[10];
  HTTP_printHeader(out,"Настройка контроллера");
  out += "\
     <ul>\
     <li><a href=\"/\">Главная</a>\
     <li><a href=\"/reboot\">Перезагрузка</a>\
     </ul>\n";

// Печатаем время в форму для корректировки

// Форма для настройки параметров
   out += "<h2>Конфигурация</h2>";
   out += "<h3>Параметры в режиме точки доступа</h3>";
   out += "<form action='/config' method='POST'><table><tr>";
   HTTP_printInput(out,"Наименование:","ESP_NAME",EC_Config.ESP_NAME,32,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"Пароль:","ESP_PASS",EC_Config.ESP_PASS,32,32,true);
   out += "</tr></table>";

   out += "<h3>Параметры подключения по WiFi</h3>";
   out += "<table><tr>";
   HTTP_printInput(out,"Сеть WiFi:","AP_SSID",EC_Config.AP_SSID,32,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"Пароль WiFi: &nbsp;&nbsp;&nbsp;","AP_PASS",EC_Config.AP_PASS,32,32,true);
   out += "</tr></table><table><tr>";
   sprintf(str,"%d",EC_Config.IP[0]); 
   HTTP_printInput(out,"Статический IP:","IP1",str,3,3);
   sprintf(str,"%d",EC_Config.IP[1]); 
   HTTP_printInput(out,".","IP2",str,3,3);
   sprintf(str,"%d",EC_Config.IP[2]); 
   HTTP_printInput(out,".","IP3",str,3,3);
   sprintf(str,"%d",EC_Config.IP[3]); 
   HTTP_printInput(out,".","IP4",str,3,3);
   out += "</tr><tr>";
   sprintf(str,"%d",EC_Config.MASK[0]); 
   HTTP_printInput(out,"Маска","MASK1",str,3,3);
   sprintf(str,"%d",EC_Config.MASK[1]); 
   HTTP_printInput(out,".","MASK2",str,3,3);
   sprintf(str,"%d",EC_Config.MASK[2]); 
   HTTP_printInput(out,".","MASK3",str,3,3);
   sprintf(str,"%d",EC_Config.MASK[3]); 
   HTTP_printInput(out,".","MASK4",str,3,3);
   out += "</tr><tr>";
   sprintf(str,"%d",EC_Config.GW[0]); 
   HTTP_printInput(out,"Шлюз","GW1",str,3,3);
   sprintf(str,"%d",EC_Config.GW[1]); 
   HTTP_printInput(out,".","GW2",str,3,3);
   sprintf(str,"%d",EC_Config.GW[2]); 
   HTTP_printInput(out,".","GW3",str,3,3);
   sprintf(str,"%d",EC_Config.GW[3]); 
   HTTP_printInput(out,".","GW4",str,3,3);
   out += "</tr></table>";
   HTTP_printInput(out,"Пароль HTTP:","HTTP_PASS","*",32,32,true);
   out += "<table><tr>";
   HTTP_printInput(out,"Сервер архива:","http_serv",EC_Config.HTTP_SERVER,32,32);
    out += "</tr><table>";

   out += "<h3>Параметры NTP</h3>";
   out += "<table><tr>";
   HTTP_printInput(out,"NTP сервер 1:","ntp_server1",EC_Config.NTP_SERVER1,32,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"NTP сервер 2:","ntp_server2",EC_Config.NTP_SERVER2,32,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"NTP сервер 3:","ntp_server3",EC_Config.NTP_SERVER3,32,32);
   out += "</tr><tr>";
   sprintf(str,"%d",EC_Config.TZ);      
   HTTP_printInput(out,"Таймзона:","tz",str,32,32,true);
   out += "</tr></table>";
   
   out +="<input type='submit' name='SUBMIT_CONF' value='Сохранить'>"; 
   out += "</form></body></html>";
   server.send ( 200, "text/html", out );
  
}        



/*
 * Сброс настрое по умолчанию
 */
void HTTP_handleDefault(void) {
// Проверка прав администратора  
  if ( HTTP_isAuth() != 0 ){
    HTTP_gotoLogin();
    return;
  } 

  EC_default();
  HTTP_handleConfig();  
}



/**
 * Проверка авторизации
 */
int HTTP_isAuth(){
//  Serial.print("AUTH ");
  if (server.hasHeader("Cookie")){   
//    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
//    Serial.print(cookie);
 
    if (cookie.indexOf("ESP_PASS=") != -1) {
      authPass = cookie.substring(cookie.indexOf("ESP_PASS=")+9);       
      return HTTP_checkAuth((char *)authPass.c_str());
    }
  }
  return -1;  
}


/**
 * Функция проверки пароля
 * возвращает 0 - админ, 1 - оператор, -1 - Не авторизован
 */
int  HTTP_checkAuth(char *pass){
   if( strcmp(pass,EC_Config.HTTP_PASS) == 0 ){ 
       UID = 0;
       HTTP_User = "Администратор";
   }
    else {
       UID = -1;
       HTTP_User = "Анонимус";
   }
//   Serial.printf("Auth is %d\n",UID);
   return UID;
}

/*
 * Перезагрузка часов
 */
void HTTP_handleReboot(void) {

  String out = "";

  out = 
"<html>\
  <head>\
    <meta charset='utf-8' />\
    <meta http-equiv='refresh' content='30;URL=/'>\
    <title>ESP8266 sensor 1</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Перезагрузка контроллера</h1>\
     <p><a href=\"/\">Через 30 сек будет переадресация на главную</a>\
   </body>\
</html>";
   server.send ( 200, "text/html", out );
   ESP.reset();  
  
}

/**
* Сохранить параметры на HTTP сервере
*/
bool SetParamHTTP(int n){
//   if( n < 0 || n >= MAX_SENSORS )return false;
   WiFiClient client;
   IPAddress ip1;
   WiFi.hostByName(EC_Config.HTTP_SERVER, ip1);
   Serial.print("IP=");
   Serial.println(ip1);
   
   String out = "";
   char str[256];
   if (!client.connect(ip1, 80)) {
       Serial.printf("Connection %s failed",EC_Config.HTTP_SERVER);
       return false;
   }
   out += "GET ";
   out += "http://";
   out += EC_Config.HTTP_SERVER;
// Формируем строку запроса  

 
   sprintf(str,"/save44.php?name=%s&uptime=%ld",EC_Config.ESP_NAME,t_cur);
   out += str;
   
   if( ds_enable ){
      sprintf(str,"&T=%02d",(int)Temp);
      out +=str;
   }
   if( dht_enable ){
      sprintf(str,"&T=%02d&H=%0d",(int)Temp,(int)Hum);
      out +=str;
   }

   out += " HTTP/1.0\r\n\r\n";
   Serial.print(out);
   client.print(out);
//   delay(100);
   return true;

   
}

