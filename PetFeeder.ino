#include <Servo.h>
#include <SimpleTimer.h>

//Wifi Libraries
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#define OPEN_POS 0
#define CLOSE_POS 160

SimpleTimer feedTimer;
int timerId;

Servo servo;

ESP8266WebServer server(80);

int feedingDelay=0;


//Function Delcarations
int handleReq(String req);
void handleRoot();
void handleNotFound();
void handleFormSubmit();
void handleOpenLid();
void handleCloseLid();
void handleRestart();
void updateTimerDuration();


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

const String HTML_BUTTONS ="<div><button onclick=\"buttonCallback(open)\" id=\"open\">Open Lid</button><button onclick=\"buttonCallback(close)\" id=\"close\">Close Lid</button><button onclick=\"buttonCallback(restart)\" id=\"restart\">Restart Timer</button></div>";
const String HTML_FORM =
"<FORM action=\"/submitSettings\" method=\"post\">"
"<P>"
"<INPUT type=\"number\" name=\"DELAY\" value=\"1\">Delay(hours)<BR>"
"<INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\">"
"</P>"
"</FORM>";

void setup() {

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



  server.on("/", handleRoot);
  server.on("/submitSettings", handleFormSubmit);
  
  server.on("/open", handleOpenLid);
  server.on("/close", handleCloseLid);
  server.on("/restart", handleRestart);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  
  //Setup Servo
  servo.attach(0);
  openLid();
  delay(30000);
  closeLid();
  

  //Setup Timers
  Serial.println("Setup Feed Timer");

  timerId = feedTimer.setInterval(120000, openLid);

}

void handleRoot() {
  
  server.send(200, "text/html", HTML_HEADER+HTML_BUTTONS+HTML_FORM+HTML_FOOTER);
  
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
  
  feedTimer.deleteTimer(timerId);
  int delayMillis = feedingDelay*60*60*1000;
  timerId = feedTimer.setInterval(delayMillis, openLid);
  }

void handleOpenLid(){
     Serial.println("HANDLE OPEN");
 }
void handleCloseLid(){
     Serial.println("HANDLE ClOSE");
 }
void handleRestart(){
     Serial.println("HANDLE RESTART");
 }



void loop() {
  feedTimer.run();
  server.handleClient();
}




void openLid() {


  Serial.println("Open Feeder");

  int currentPosition = servo.read();
  Serial.println("Current Pos");
  Serial.println(currentPosition);
  for (int pos = currentPosition ; pos >= OPEN_POS; pos -= 1) { // goes from 0 degrees to 180 degrees
    Serial.println("-");
    Serial.print("Current Pos");
    Serial.print( servo.read());

    // in steps of 1 degree
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  currentPosition = servo.read();
  servo.write(currentPosition -0.5f);
  delay(15);     
}


void closeLid() {
  Serial.println("Close Feeder");

  int currentPosition = servo.read();
  Serial.println("Current Pos");
  Serial.println(currentPosition);
  for (int pos = currentPosition ; pos <= CLOSE_POS; pos += 1) { // goes from 0 degrees to 180 degrees
    Serial.println("+");
    Serial.print("Current Pos");
    Serial.print( servo.read());

    // in steps of 1 degree
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }

currentPosition = servo.read();
  servo.write(currentPosition +0.5f);
  delay(15);     


}




