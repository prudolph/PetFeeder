
#include "SimpleTimer.h"
#include <Servo.h>
#include <EEPROM.h>
//Wifi Libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
//https://randomnerdtutorials.com/esp8266-web-server-spiffs-nodemcu/
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESP8266HTTPClient.h>

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <Time.h>



// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");


SimpleTimer feedTimer;
unsigned long timerStartTime; 
int timerId;


// ConfigData
  struct { 
    int openPos = 5;
    int closePos = 155;
  } configData;
uint configDataAddress = 0;


Servo servo;

AsyncWebServer server(80);

int feedingDelay=24;
bool SMSNotificationEnabled=false;
String smsUrl="http://maker.ifttt.com/trigger/pet_fed/with/key//";

//Function Delcarations
int handleReq(String req);
void handleRoot();
void handleNotFound();
void handleFormSubmit();
void handleOpenLid();
void handleCloseLid();
void handleRestart();
void updateTimerDuration();
void sendSMS();
unsigned long getRemainingTime();

void setup() {

    Serial.begin(115200);
    delay(500);
    

  timerId = feedTimer.setInterval(120000, openLid);
  updateTimerDuration();
  


////  WiFiManager wifiManager;
//  wifiManager.autoConnect("CatFeeder_0", "password");
//  wifiManager.setConfigPortalTimeout(30);

 Serial.println("!!!!connected-------------)");

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Netmask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());

//ReadConfigSettings:
  EEPROM.begin(512);
  EEPROM.get(configDataAddress,configData);
  Serial.println("Old values are: openpos"+String(configData.openPos)+", closePos"+String(configData.closePos));


if(!SPIFFS.begin()){
  Serial.println("An Error has occurred while mounting SPIFFS");
 
}


// Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(-25200);


  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });

  //Dispense Control APIS 
  server.on("/api/open", HTTP_GET, [](AsyncWebServerRequest *request){
    openLid();
    request->send(200, "text/json","{\"task\":\"open\",\"status\":\"complete\"}");
  });
  
  server.on("/api/close", HTTP_GET, [](AsyncWebServerRequest *request){
      closeLid();
     request->send(200, "text/json","{\"task\":\"close\",\"status\":\"complete\"}");
  });

   server.on("/api/dispense", HTTP_GET, [](AsyncWebServerRequest *request){
      closeLid();
      delay(500);
      openLid();
      delay(500);
      closeLid();
      
     request->send(200, "text/json","{'task':'dispense','status':'complete'}");
  });


   server.on("/api/openpos", HTTP_GET, [](AsyncWebServerRequest *request){  
     request->send(200, "text/json","{\"task\":\"getopenpos\",\"result\":\""+String(configData.openPos)+"\"}");
  });
    server.on("/api/closepos", HTTP_GET, [](AsyncWebServerRequest *request){  
     request->send(200, "text/json","{\"task\":\"getopenpos\",\"result\":\""+String(configData.closePos)+"\"}");
  });


    server.on("/api/openpos", HTTP_POST, [](AsyncWebServerRequest *request){  
      int params = request->params();      
      AsyncWebParameter* p = request->getParam(0);
      //Serial.printf("POST PARAM [%s]: %s\n", p->name().c_str(), p->value().c_str());
      int paramVal= atoi(p->name().c_str());
      configData.openPos = paramVal;
      EEPROM.put(configDataAddress, configData);
      EEPROM.commit();  
      //Serial.println("Old values are: openpos"+String(configData.openPos)+", closePos"+String(configData.closePos));
      request->send(200, "text/json","{\"task\":\"setopenpos\",\"result\":\""+String(configData.openPos)+"\"}");
  });



    server.on("/api/closepos", HTTP_POST, [](AsyncWebServerRequest *request){  
      int params = request->params();      
      AsyncWebParameter* p = request->getParam(0);
      //Serial.printf("POST PARAM [%s]: %s\n", p->name().c_str(), p->value().c_str());
       int paramVal= atoi(p->name().c_str());
       configData.closePos = paramVal;
       EEPROM.put(configDataAddress, configData);
       EEPROM.commit();  
       request->send(200, "text/json","{\"task\":\"setclosepos\",\"result\":\""+String(configData.closePos)+"\"}");
  });




server.on("/api/time", HTTP_GET, [](AsyncWebServerRequest *request){
      unsigned long epochTime = timeClient.getEpochTime();
      Serial.print("Epoch Time: ");
      Serial.println(epochTime);

      char buf[60];
      sprintf(buf, "{\"task\":\"open\",\"status\":\"%lu\"}", epochTime);
      
     request->send(200, "text/json",buf);
  });

 
//  server.on("/submitSettings", handleFormSubmit);  
//  server.on("/open", handleOpenLid);
//  server.on("/close", handleCloseLid);
//  server.on("/restart", handleRestart);
//  server.onNotFound(handleNotFound);


  server.begin();

  
}

//void handleRoot() {
//  unsigned long remainingTime = getRemainingTime();
//  String timeRemaining= "Remaining Time Hours: "+String(remainingTime/(60*60*1000))+ " Minutes:"+ String((remainingTime/(60*1000))%60);
//  server.send(200, "text/html", HTML_HEADER+"<div class=\"time\">"+timeRemaining +"<div>" +HTML_BUTTONS+HTML_FORM+HTML_FOOTER);
//
//
//
//  Serial.println(timeRemaining);
//}

//void handleNotFound() {
//  
//  String message = "File Not Found\n\n";
//  message += "URI: ";
//  message += server.uri();
//  message += "\nMethod: ";
//  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
//  message += "\nArguments: ";
//  message += server.args();
//  message += "\n";
//
//  for ( uint8_t i = 0; i < server.args(); i++ ) {
//    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
//  }
//
//  server.send ( 404, "text/plain", message );
//  
//}

// Replaces placeholder with LED state value
String processor(const String& var){

  return "from processor";
//  
//  Serial.println(var);
//  if(var == "STATE"){
//    if(digitalRead(ledPin)){
//      ledState = "ON";
//    }
//    else{
//      ledState = "OFF";
//    }
//    Serial.print(ledState);
//    return ledState;
//  }
//  else if (var == "TEMPERATURE"){
//    return getTemperature();
//  }
//  else if (var == "HUMIDITY"){
//    return getHumidity();
//  }
//  else if (var == "PRESSURE"){
//    return getPressure();
//  }  
}

//
//void handleFormSubmit(){
//    Serial.println("Form Submitted");
//    String feedingDelayValue  = server.arg("DELAY");
//    feedingDelay=feedingDelayValue.toInt();
//    Serial.println("Feeding Delay(hours): ");
//    Serial.println(feedingDelay);
//    updateTimerDuration();
//    
//}
//
void updateTimerDuration(){
  SMSNotificationEnabled=true;
  feedTimer.deleteTimer(timerId);
  int delayMillis = feedingDelay*60*60*1000;
  timerId = feedTimer.setInterval(delayMillis, openLid);

  feedTimer.restartTimer(timerId);
  timerStartTime = millis();
  String response = " Timer Set for "+ String(feedingDelay)+ " Hours";
  Serial.println("Timer Started");
  //server.send(200, "text/plain", response);
 }


//void handleRestart(){
//     Serial.println("HANDLE RESTART");
//     feedTimer.restartTimer(timerId);
//     timerStartTime = millis();
//     Serial.println("Timer Restarted");
//     server.send(200, "text/plain", "Timer Restarted");
//     SMSNotificationEnabled=true;
// }

unsigned long getRemainingTime(){
  
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - timerStartTime;
  unsigned long feedingDelayMillis= feedingDelay*60 * 60 * 1000;
  
  return feedingDelayMillis-elapsedTime;
}

void loop() {
   timeClient.update();
  feedTimer.run();
  //server.handleClient();
}




void openLid() {

  servo.attach(0);
  
  Serial.println("Open Feeder");

  int currentPosition = servo.read();
  Serial.println("Current Pos");
  Serial.println(currentPosition);
  for (int pos = currentPosition ; pos >= configData.openPos; pos -= 1) { // goes from 0 degrees to 180 degrees
    Serial.println("-");
    Serial.print("Current Pos");
    Serial.print( pos);
    if(pos == configData.openPos) break;
    // in steps of 1 degree
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  
  //Trying to stop the servo whine
   delay(500);   
  
  currentPosition = servo.read();
  for (int pos = currentPosition ; pos <= configData.openPos+60; pos += 1) { // goes from 0 degrees to 180 degrees
    Serial.println("-");
    Serial.print("Current Pos");
    Serial.print( pos);
    if(pos == configData.openPos+60) break;
    // in steps of 1 degree
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  //sendSMS();
  servo.detach();
}


void closeLid() {
    
  servo.attach(0);
  Serial.println("Close Feeder");

  int currentPosition = servo.read();
  Serial.println("Current Pos");
  Serial.println(currentPosition);
  for (int pos = currentPosition ; pos <= configData.closePos; pos += 1) { // goes from 0 degrees to 180 degrees
    Serial.println("+");
    Serial.print("Current Pos");
    Serial.print( servo.read());
    if(pos == configData.closePos) break;
    // in steps of 1 degree
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
     
  }
  
  servo.detach();
}


void sendSMS(){
  if(! SMSNotificationEnabled )return;
    Serial.print("Sending SMS");
    HTTPClient http;
    http.begin(smsUrl);
    int httpCode = http.GET();
    if(httpCode > 0) {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if(httpCode == HTTP_CODE_OK) {
              String payload = http.getString();
              Serial.println(payload);
      }
    } else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
  http.end();
     
  }
