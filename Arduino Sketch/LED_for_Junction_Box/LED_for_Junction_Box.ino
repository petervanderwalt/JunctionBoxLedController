#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "index.h" //Our HTML webpage contents with javascripts

/* PIN Definition */
#define DOORBELL_SWITCH 5
#define MOTION_SENSOR 4
#define LED_STRING 12
#define DOORBELL_SWITCH_LED 13
#define SIREN 14
#define BOOT_BUTTON 0
#define ONBOARD_LED 2
bool doorBellRang = false;
bool motionDetected = false;

#ifndef STASSID
#define STASSID "30Bird-Stoep"
#define STAPSK  "aabbccddeeff"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 60000);

#include <UniversalTelegramBot.h>
#define BOTtoken "1002866355:AAHytxbzXvwG8OQOusV5YTTtwMNcLVGUGko"  // your Bot Token (Get from Botfather)
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;



#include <Ticker.h>
Ticker ticker;
void tick() {
  //toggle state
  digitalWrite(DOORBELL_SWITCH_LED, !digitalRead(DOORBELL_SWITCH_LED));     // set pin to the opposite state
}

void ntpTick() {
    timeClient.update();
}


//===============================================================
// Theres routines are executed when you open its IP in browser
//===============================================================
void handleRoot() {
  String s = MAIN_page; //Read HTML contents
  httpServer.send(200, "text/html", s); //Send web page
}

void handleADC() {
  int a = analogRead(A0);
  String adcValue = String(a);
  httpServer.send(200, "text/plain", adcValue);
}

void compileTime() {
  int a = analogRead(A0);
  String compileTime = String(F(__DATE__)) + String(" ") + String(F(__TIME__));
  httpServer.send(200, "text/plain", compileTime);
}

void sendTestMsg() {
    bot.sendMessage("@the30bird", "TEST: This is a test", "");
    httpServer.send(200, "text/plain", "Sent!");
}

void sendPicMsg() {
    String test_photo_url = "http://192.168.88.100/picture";
    bot.sendPhoto("@the30bird", test_photo_url, "Caption is optional, you may not use photo caption");
    httpServer.send(200, "text/plain", "Sent!");
}


void getTime() {
  httpServer.send(200, "text/plain", timeClient.getFormattedTime());
}

//===============================================================
// Interrupt Routings
//===============================================================

ICACHE_RAM_ATTR void handleInterruptMotionSensor() {
  motionDetected = true;
}
ICACHE_RAM_ATTR void handleInterruptDoorBell() {
  doorBellRang = true;
}


//===============================================================
// Setup
//===============================================================

void setup(void) {

  pinMode(LED_STRING, OUTPUT);
  pinMode(DOORBELL_SWITCH_LED, OUTPUT);
  pinMode(SIREN, OUTPUT);

  digitalWrite(LED_STRING, HIGH);

  pinMode(MOTION_SENSOR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MOTION_SENSOR), handleInterruptMotionSensor, FALLING);

  pinMode(DOORBELL_SWITCH, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DOORBELL_SWITCH), handleInterruptDoorBell, FALLING);

  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting...");
  Serial.print("Compiled: ");
  Serial.print (F(__DATE__)) ;
  Serial.print (" ") ;
  Serial.println (F(__TIME__)) ;

  WiFi.hostname("Doorbell");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }
  Serial.println(WiFi.localIP());

  httpServer.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  httpServer.on("/readADC", handleADC); //This page is called by java Script AJAX
  httpServer.on("/compileTime", compileTime); //This page is called by java Script AJAX
  httpServer.on("/testMessage", sendTestMsg);
  httpServer.on("/testCamera", sendPicMsg);

  httpServer.on("/getTime", getTime);
  
  httpServer.begin();

  httpUpdater.setup(&httpServer);

  client.setInsecure();

  timeClient.begin();

  bot.sendMessage("@the30bird", "Doorbell Module started up", "");
  ticker.attach(10, ntpTick);
}


//===============================================================
// Loop
//===============================================================

void loop(void) {

  if (doorBellRang) {
    ticker.attach(0.2, tick);
    // String keyboardJson = "[[{ \"text\" : \"I'm coming\", \"callback_data\" : \"/coming\" }],[{ \"text\" : \"Come back Later\", \"callback_data\" : \"/later\" }],[{ \"text\" : \"ALARM\", \"callback_data\" : \"/alarm\" }]]";
    // bot.sendMessageWithInlineKeyboard("@the30bird", "DOORBELL: Front door", "", keyboardJson);
    bot.sendMessage("@the30bird", "DOORBELL: Front Door", "");

    ticker.detach();
    doorBellRang = false;
  }

  if (motionDetected) {
    ticker.attach(0.2, tick);
    // String keyboardJson = "[[{ \"text\" : \"I'm coming\", \"callback_data\" : \"/coming\" }],[{ \"text\" : \"Come back Later\", \"callback_data\" : \"/later\" }],[{ \"text\" : \"ALARM\", \"callback_data\" : \"/alarm\" }]]";
    // bot.sendMessageWithInlineKeyboard("@the30bird", "DOORBELL: Front door", "", keyboardJson);

    bot.sendMessage("@the30bird", "ALARM: Motion Detected at 32 Bird Street", "");

    digitalWrite(SIREN, HIGH);
    delay(3000);
    digitalWrite(SIREN, LOW);
    ticker.detach();
    motionDetected = false;
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
