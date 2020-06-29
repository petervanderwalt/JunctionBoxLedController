/*
  To upload through terminal you can use: curl -F "image=@firmware.bin" esp8266-webupdate.local/update
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#include "index.h" //Our HTML webpage contents with javascripts


#ifndef STASSID
#define STASSID "BRAAI-FI"
#define STAPSK  "aabbccddeeff"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

const int redLED = 12;   
const int greenLED = 13; 
const int blueLED = 14;  

//===============================================================
// This routines are executed when you open its IP in browser
//===============================================================
void handleRoot() {
 String s = MAIN_page; //Read HTML contents
 httpServer.send(200, "text/html", s); //Send web page
}

void handleADC() {
 int a = analogRead(A0);
 String adcValue = String(a);
 httpServer.send(200, "text/plane", adcValue); //Send ADC value only to client ajax request
}

void compileTime() {
 int a = analogRead(A0);
 String compileTime = String(F(__DATE__)) + String(" ") + String(F(__TIME__));
 httpServer.send(200, "text/plane", compileTime); //Send ADC value only to client ajax request
}

void rgbHandler() {
  // Serial.println("handle root..");
String red = httpServer.arg(0); // read RGB arguments
String green = httpServer.arg(1);
String blue = httpServer.arg(2);

if((red != "") && (green != "") && (blue != ""))
{
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

httpServer.send(200, "text/html", rgbpage);
}



void setup(void) {

  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting...");
  Serial.print("Compiled: ");
  Serial.print (F(__DATE__)) ;
  Serial.print (" ") ;
  Serial.println (F(__TIME__)) ;
 
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
  httpServer.on("/compileTime", compileTime); //This page is called by java Script AJAX
  httpServer.on("/rgb", rgbHandler); //This page is called by java Script AJAX
  httpServer.begin();

  Serial.printf("HTTPUpdateServer ready!");
}

void loop(void) {
  httpServer.handleClient(); 
}
