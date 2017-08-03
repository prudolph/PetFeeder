#include <Servo.h>
#include <SimpleTimer.h>

//Wifi Libraries
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266mDNS.h>
#include <Time.h>

#define OPEN_POS 5
#define CLOSE_POS 155


SimpleTimer feedTimer;
unsigned long timerStartTime; 
int timerId;

Servo servo;

ESP8266WebServer server(80);

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

const String HTML_HEADER =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
"<title>Feeder Configuration</title>"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
"</style>"
"<script>"
 "function buttonCallback(String mode ){"
 "console.log(\"Mode\"+mode);"
 "var xhttp = new XMLHttpRequest();"
 "xhttp.onreadystatechange = function() {"
 "xhttp.open(\"GET\", mode, true); xhttp.send();"
 "}"
"</script>"
"</head>"
"<body>";


const String HTML_FOOTER = "</body></html>";

const String HTML_BUTTONS ="<div><button onclick=\"location.href='/open';\" id=\"open\">Open Lid</button><button  onclick=\"location.href='/close';\" id=\"close\">Close Lid</button><button onclick=\"location.href='/restart';\" id=\"restart\">Restart Timer</button></div>";
const String HTML_FORM =
"<FORM action=\"/submitSettings\" method=\"post\">"
"<P>"
"<INPUT type=\"number\" name=\"DELAY\" value=\"1\">Delay(hours)<BR>"
"<INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\">"
"</P>"
"</FORM>";

void setup() {
  //Setup Servo
  openLid();
  
  timerId = feedTimer.setInterval(120000, openLid);
  updateTimerDuration();
  
  Serial.begin(115200);
  delay(100);

  WiFiManager wifiManager;
  wifiManager.autoConnect("CatFeeder_0", "password");
  wifiManager.setConfigPortalTimeout(30);

 Serial.println("connected-------------)");

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Netmask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());

 if (MDNS.begin("CatFeeder")) {  //Start mDNS
  
}

  server.on("/", handleRoot);
  server.on("/submitSettings", handleFormSubmit);
  
  server.on("/open", handleOpenLid);
  server.on("/close", handleCloseLid);
  server.on("/restart", handleRestart);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  
}

void handleRoot() {
  unsigned long remainingTime = getRemainingTime();
  String timeRemaining= "Remaining Time Hours: "+String(remainingTime/(60*60*1000))+ " Minutes:"+ String((remainingTime/(60*1000))%60);
  server.send(200, "text/html", HTML_HEADER+"<div class=\"time\">"+timeRemaining +"<div>" +HTML_BUTTONS+HTML_FORM+HTML_FOOTER);



  Serial.println(timeRemaining);
}

void handleNotFound() {
  
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
  
}
void handleFormSubmit(){
    Serial.println("Form Submitted");
    String feedingDelayValue  = server.arg("DELAY");
    feedingDelay=feedingDelayValue.toInt();
    Serial.println("Feeding Delay(hours): ");
    Serial.println(feedingDelay);
    updateTimerDuration();
    
}

void updateTimerDuration(){
  SMSNotificationEnabled=true;
  feedTimer.deleteTimer(timerId);
  int delayMillis = feedingDelay*60*60*1000;
  timerId = feedTimer.setInterval(delayMillis, openLid);

  feedTimer.restartTimer(timerId);
  timerStartTime = millis();
  String response = " Timer Set for "+ String(feedingDelay)+ " Hours";
  Serial.println("Timer Started");
  server.send(200, "text/plain", response);
 }

void handleOpenLid(){
     Serial.println("HANDLE OPEN");
     SMSNotificationEnabled=false;
     openLid();
     SMSNotificationEnabled=true;
     server.send(200, "text/plain", "Opened");
 }
void handleCloseLid(){
     Serial.println("HANDLE ClOSE");
      closeLid();
      server.send(200, "text/plain", "Closed");
 }
void handleRestart(){
     Serial.println("HANDLE RESTART");
     feedTimer.restartTimer(timerId);
     timerStartTime = millis();
     Serial.println("Timer Restarted");
     server.send(200, "text/plain", "Timer Restarted");
     SMSNotificationEnabled=true;
 }

unsigned long getRemainingTime(){
  
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - timerStartTime;
  unsigned long feedingDelayMillis= feedingDelay*60 * 60 * 1000;
  
  return feedingDelayMillis-elapsedTime;
}

void loop() {
  feedTimer.run();
  server.handleClient();
}




void openLid() {

  servo.attach(0);
  
  Serial.println("Open Feeder");

  int currentPosition = servo.read();
  Serial.println("Current Pos");
  Serial.println(currentPosition);
  for (int pos = currentPosition ; pos >= OPEN_POS; pos -= 1) { // goes from 0 degrees to 180 degrees
    Serial.println("-");
    Serial.print("Current Pos");
    Serial.print( pos);
    if(pos == OPEN_POS) break;
    // in steps of 1 degree
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  
  //Trying to stop the servo whine
   delay(500);   
  
  currentPosition = servo.read();
  for (int pos = currentPosition ; pos <= OPEN_POS+60; pos += 1) { // goes from 0 degrees to 180 degrees
    Serial.println("-");
    Serial.print("Current Pos");
    Serial.print( pos);
    if(pos == OPEN_POS+60) break;
    // in steps of 1 degree
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  sendSMS();
  servo.detach();
 
}


void closeLid() {
    
  servo.attach(0);
  Serial.println("Close Feeder");

  int currentPosition = servo.read();
  Serial.println("Current Pos");
  Serial.println(currentPosition);
  for (int pos = currentPosition ; pos <= CLOSE_POS; pos += 1) { // goes from 0 degrees to 180 degrees
    Serial.println("+");
    Serial.print("Current Pos");
    Serial.print( servo.read());
    if(pos == CLOSE_POS) break;
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


