/*
  To upload through terminal you can use: curl -F "image=@firmware.bin" esp8266-webupdate.local/update
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#include "index.h" //Our HTML webpage contents with javascripts

#ifndef STASSID
#define STASSID "30Bird-Stoep"
#define STAPSK  "aabbccddeeff"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

const int redLED = 12;   
const int greenLED = 13; 
const int blueLED = 14;  

//#include <UniversalTelegramBot.h>
//#define BOTtoken "1002866355:AAHytxbzXvwG8OQOusV5YTTtwMNcLVGUGko"  // your Bot Token (Get from Botfather)
//WiFiClientSecure client;
//UniversalTelegramBot bot(BOTtoken, client);
//int botRequestDelay = 1000;
//unsigned long lastTimeBotRan;


#define pushButton 0
#define onboardLED 2
bool buttonPushed = false;

#include <Ticker.h>
Ticker ticker;
void tick() {
  //toggle state
  digitalWrite(onboardLED, !digitalRead(onboardLED));     // set pin to the opposite state
}

#include <Wire.h>

//===============================================================
// This routines are executed when you open its IP in browser
//===============================================================
void handleRoot() {
  // Serial.println("handle root..");
  String red = httpServer.arg(0); // read RGB arguments
  String green = httpServer.arg(1);
  String blue = httpServer.arg(2);

  if((red != "") && (green != "") && (blue != "")) {
    analogWrite(redLED, 1023 - red.toInt());
    analogWrite(greenLED, 1023 - green.toInt());
    analogWrite(blueLED, 1023 - blue.toInt());
  }


  Serial.print("Red: ");
  Serial.println(red.toInt()); 
  Serial.print("Green: ");
  Serial.println(green.toInt()); 
  Serial.print("Blue: ");
  Serial.println(blue.toInt()); 
  Serial.println();

 String s = MAIN_page; //Read HTML contents
 httpServer.send(200, "text/html", s); //Send web page
}

void handleADC() {
 int a = analogRead(A0);
 String adcValue = String(a);
 httpServer.send(200, "text/plain", adcValue); //Send ADC value only to client ajax request
}

void handleD4() {
 int d4 = digitalRead(4);
 String d4Value = String(d4);
 httpServer.send(200, "text/plain", d4Value); //Send ADC value only to client ajax request
}

void handleD5() {
 int d5 = digitalRead(5);
 String d5Value = String(d5); 
 httpServer.send(200, "text/plain", d5Value); //Send ADC value only to client ajax request
}

void handlei2cscan() {

  byte error, address;
  int nDevices;
  nDevices = 0;
  String message = "Scanning: \n\n";
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0) {
      message += "I2C device found at address 0x";
      Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
      message += String(address, HEX);
      message += "\n";
      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknown error at address 0x");
      message+= "Unknown error at address 0x";
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
      message += String(address, HEX);
      message += "\n";
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
    message += "No I2C devices found\n";
  } else {
    Serial.println("done\n");
    message += "done\n";
  }

  httpServer.send(200, "text/plain", message); //Send ADC value only to client ajax request
}

void compileTime() {
 int a = analogRead(A0);
 String compileTime = String(F(__DATE__)) + String(" ") + String(F(__TIME__));
 httpServer.send(200, "text/plain", compileTime); //Send ADC value only to client ajax request
}


ICACHE_RAM_ATTR void handleInterrupt() {
  buttonPushed = true;
}

void setup(void) {

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  digitalWrite(redLED, HIGH);
  digitalWrite(greenLED, HIGH);
  digitalWrite(blueLED, HIGH);
  
  Wire.begin();
 
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting...");
  Serial.print("Compiled: ");
  Serial.print (F(__DATE__)) ;
  Serial.print (" ") ;
  Serial.println (F(__TIME__)) ;
  
  WiFi.hostname("Doorbell");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }

  Serial.println(WiFi.localIP());

  httpUpdater.setup(&httpServer);

  httpServer.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  httpServer.on("/readADC", handleADC); //This page is called by java Script AJAX
  httpServer.on("/readD4", handleD4); //This page is called by java Script AJAX
  httpServer.on("/readD5", handleD5); //This page is called by java Script AJAX
  httpServer.on("/compileTime", compileTime); //This page is called by java Script AJAX
  httpServer.on("/i2cscan", handlei2cscan);
  httpServer.begin();

  Serial.printf("HTTPUpdateServer ready!");

//  client.setInsecure();

  pinMode(pushButton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pushButton), handleInterrupt, RISING);

  pinMode(onboardLED, OUTPUT);
  digitalWrite(onboardLED, HIGH);
}

void loop(void) {

  if (buttonPushed) {
    digitalWrite(onboardLED, LOW);
    ticker.attach(0.2, tick);
    Serial.println("Button pressed - sending telegram");
//    String keyboardJson = "[[{ \"text\" : \"I'm coming\", \"callback_data\" : \"/coming\" }],[{ \"text\" : \"Come back Later\", \"callback_data\" : \"/later\" }],[{ \"text\" : \"ALARM\", \"callback_data\" : \"/alarm\" }]]";
//    bot.sendMessageWithInlineKeyboard("@the30bird", "DOORBELL: Front door", "", keyboardJson);
    buttonPushed = false;
    ticker.detach();
    digitalWrite(onboardLED, HIGH);
  }

//  if (millis() > lastTimeBotRan + botRequestDelay)  {
//    
//    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
//    while (numNewMessages) {
//      Serial.println("got response");
//      for (int i = 0; i < numNewMessages; i++) {
//        if (bot.messages[i].type == "callback_query") {
//          Serial.print("Callback button pressed by: ");
//          Serial.println(bot.messages[i].from_id);
//          Serial.print("Action: ");
//          Serial.println(bot.messages[i].text);
//          if (bot.messages[i].text == "/alarm") {
//            digitalWrite(12, HIGH);
//            delay(5000);
//            digitalWrite(12, LOW);
//          } else if (bot.messages[i].text == "/coming") {
//            digitalWrite(13, HIGH);
//            delay(5000);
//            digitalWrite(13, LOW);
//          } else if (bot.messages[i].text == "/later") {
//            digitalWrite(14, HIGH);
//            delay(5000);
//            digitalWrite(14, LOW);
//          }
//          bot.sendMessage("@the30bird", bot.messages[i].text, "");
//        } else {
//          if (bot.messages[i].text == "/alarm") {
//            digitalWrite(12, HIGH);
//            delay(5000);
//            digitalWrite(12, LOW);
//          }
//        }
//      }
//      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
//    }
//
//    lastTimeBotRan = millis();
//  }
  httpServer.handleClient(); 
}
