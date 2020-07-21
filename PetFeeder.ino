

#include <Servo.h>


//Wifi Libraries
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>// for get request
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

//Data Storage
#include <FS.h>
#include <EEPROM.h>

//NTP Libraries
#include <NTPClient.h>//https://randomnerdtutorials.com/esp8266-web-server-spiffs-nodemcu/
#include <WiFiUdp.h>

#include <Time.h>

#define MAX_FEEDINGS 10


Servo servo;
AsyncWebServer server(80);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char* ssid     = "beard_ubt";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "Northstar1";     // The password of the Wi-Fi network




// ConfigData
struct {
  int openPos = 5;
  int closePos = 155;
  int servoSpeed = 15;
} configData;
uint configDataAddress = 0;

// FeedDateStruct
typedef struct {
  time_t feedDate = 0;
  bool daily = false;
  bool completed = false;
} FeedDate;

int feedDatesCount = 0;
uint feedingCountAddress = sizeof(configData) + 1;

FeedDate feedDates[MAX_FEEDINGS];
uint feedingDataAddress = sizeof(configData) + sizeof(feedDatesCount) + 2;


String smsUrl = "http://maker.ifttt.com/trigger/pet_fed/with/key//";


//Function Delcarations
void setupServer();

void sendSMS();
unsigned long getRemainingTime();

void setup() {

  Serial.begin(115200);
  delay(500);
     
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  //ReadConfigSettings:
  EEPROM.begin(1024);
  EEPROM.get(configDataAddress, configData);
  delay(100);
  
  if (configData.openPos == -1)    configData.openPos = 0;
  if (configData.closePos == -1)   configData.closePos = 180;
  if (configData.servoSpeed == -1) configData.servoSpeed = 15;

  Serial.println("Old values are: openpos" + String(configData.openPos) + ", closePos" + String(configData.closePos) + ", servospeed" + String(configData.servoSpeed));

  //get feeddatescount
  EEPROM.get( feedingCountAddress , feedDatesCount );
  //get feeddates
  EEPROM.get( feedingDataAddress , feedDates );

  //TESTING ONLY 
  configData.openPos = 0;
  configData.closePos = 180;
   
  if (!SPIFFS.begin()) {Serial.println("An Error has occurred while mounting SPIFFS");}


  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(0);

  setupServer();

}


void setupServer(){
  
  
  

//Setup Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html");
  });

   //Dispense Control APIS
  server.on("/api/open", HTTP_GET, [](AsyncWebServerRequest * request) {
    openLid();
    request->send(200, "text/json", "{\"task\":\"open\",\"status\":\"complete\"}");
  });

  server.on("/api/close", HTTP_GET, [](AsyncWebServerRequest * request) {
    closeLid();
    request->send(200, "text/json", "{\"task\":\"close\",\"status\":\"complete\"}");
  });

  server.on("/api/dispense", HTTP_GET, [](AsyncWebServerRequest * request) {
    closeLid(); delay(500);
    openLid();  delay(500);
    closeLid();
    request->send(200, "text/json", "{'task':'dispense','status':'complete'}");
  });



//Time APIS

  server.on("/api/time", HTTP_GET, [](AsyncWebServerRequest * request) {
    time_t epochTime = timeClient.getEpochTime();
    char buf[60];
    sprintf(buf, "{\"task\":\"open\",\"status\":\"%lu\"}", epochTime);
    request->send(200, "text/json", buf);
  });





  server.on("/api/openpos", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/json", "{\"task\":\"getopenpos\",\"result\":\"" + String(configData.openPos) + "\"}");
  });
  server.on("/api/closepos", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/json", "{\"task\":\"getopenpos\",\"result\":\"" + String(configData.closePos) + "\"}");
  });


  server.on("/api/openpos", HTTP_POST, [](AsyncWebServerRequest * request) {
    int params = request->params();
    AsyncWebParameter* p = request->getParam(0);
    //Serial.printf("POST PARAM [%s]: %s\n", p->name().c_str(), p->value().c_str());
    int paramVal = atoi(p->name().c_str());
    configData.openPos = paramVal;

    setLidPos(  configData.openPos);
    EEPROM.put(configDataAddress, configData);
    EEPROM.commit();
    Serial.println("Old values are: openpos" + String(configData.openPos) + ", closePos" + String(configData.closePos));
    request->send(200, "text/json", "{\"task\":\"setopenpos\",\"result\":\"" + String(configData.openPos) + "\"}");
  });



  server.on("/api/closepos", HTTP_POST, [](AsyncWebServerRequest * request) {
    int params = request->params();
    AsyncWebParameter* p = request->getParam(0);
    //Serial.printf("POST PARAM [%s]: %s\n", p->name().c_str(), p->value().c_str());
    int paramVal = atoi(p->name().c_str());
    configData.closePos = paramVal;
     setLidPos(  configData.closePos);
    EEPROM.put(configDataAddress, configData);
    EEPROM.commit();
    Serial.println("Old values are: openpos" + String(configData.openPos) + ", closePos" + String(configData.closePos));
    request->send(200, "text/json", "{\"task\":\"setclosepos\",\"result\":\"" + String(configData.closePos) + "\"}");
  });




  server.on("/api/time", HTTP_GET, [](AsyncWebServerRequest * request) {
    time_t epochTime = timeClient.getEpochTime();
    Serial.print("Epoch Time: ");
    Serial.println(epochTime);

    char buf[60];
    sprintf(buf, "{\"task\":\"open\",\"status\":\"%lu\"}", epochTime);
    request->send(200, "text/json", buf);
  });


 
  server.on("/api/addfeeddate", HTTP_POST, [](AsyncWebServerRequest * request) {
    int params = request->params();
    AsyncWebParameter* p = request->getParam(0);
    //Serial.printf("POST PARAM [%s]: %s\n", p->name().c_str(), p->value().c_str());
    time_t epochVal = atol(p->name().c_str());
    int index = addfeeding(epochVal, false);

    char buf[100];
    sprintf(buf, "{\"task\":\"addfeedingtime\",\"status\":\"success\",\"index\":\"%d\" }", index);
    request->send(200, "text/json", buf);
  });

  server.on("/api/feeddates", HTTP_GET, [](AsyncWebServerRequest * request) {
    String currentFeedDates="[";
    for (int i = 0; i < feedDatesCount; i++) {  
      time_t timestamp = feedDates[i].feedDate;
      bool daily = feedDates[i].daily;
      bool completed = feedDates[i].completed;
      currentFeedDates+="{\"index\":"+String(i)+",\"time\":"+String(timestamp)+",\"daily\":"+String(daily)+"},";
    }
 
    currentFeedDates=currentFeedDates.substring(0,currentFeedDates.length() -1);
    currentFeedDates+="]";
    char buf[100];
    sprintf(buf, "{\"task\":\"addfeedingtime\",\"status\":\"success\" }");
    request->send(200, "text/json", "{\"task\":\"getfeedingtimes\",\"result\":"+currentFeedDates+"}");
  });

 server.on("/api/removefeeddate", HTTP_POST, [](AsyncWebServerRequest * request) {
    int params = request->params();
    AsyncWebParameter* p = request->getParam(0);
    //Serial.printf("POST PARAM [%s]: %s\n", p->name().c_str(), p->value().c_str());
    int indextoremove = atol(p->name().c_str());
    removefeeding(indextoremove);
    char buf[100];
    sprintf(buf, "{\"task\":\"removefeedingtime\",\"status\":\"success\",\"index\":\"%d\" }", indextoremove);
    request->send(200, "text/json", buf);
  });
  
  server.on("/api/clearfeedings", HTTP_POST, [](AsyncWebServerRequest * request) {

    for (int i = 0; i < MAX_FEEDINGS; i++) {
      feedDates[i].feedDate = 0;
      feedDates[i].daily = false;
    }
    feedDatesCount = 0;
    EEPROM.put( feedingCountAddress , feedDatesCount );
    EEPROM.put( feedingDataAddress , feedDates );
    EEPROM.commit();
    request->send(200, "text/json", "{\"task\":\"clearfeedings\",\"status\":\"success\" }");
  });


  server.on("/api/servospeed", HTTP_POST, [](AsyncWebServerRequest * request) {

    int params = request->params();
    AsyncWebParameter* p = request->getParam(0);
    Serial.printf("POST PARAM [%s]: %s\n", p->name().c_str(), p->value().c_str());
    int paramVal = atoi(p->name().c_str());

    Serial.print("SETTING SERVO SPEED");
    Serial.print(paramVal);
    
    configData.servoSpeed = paramVal;

    EEPROM.put(configDataAddress, configData);
    EEPROM.commit();
    
    request->send(200, "text/json",  "{\"task\":\"setspeed\",\"result\":\""+ String(configData.servoSpeed) +"\"}");
  });

  server.begin();
 
  }

int addfeeding(time_t feedingTimestamp, bool daily ) {
  Serial.print("adding feeding time: ");
  Serial.println(feedingTimestamp);
  if (feedDatesCount + 1 <= MAX_FEEDINGS) {
    FeedDate newFeeding;
    newFeeding.feedDate = feedingTimestamp;
    newFeeding.daily = daily;
    newFeeding.completed = false;
    feedDates[feedDatesCount++] = newFeeding;

    EEPROM.put(feedingCountAddress, feedDatesCount);
    EEPROM.put(feedingDataAddress, feedDates);
    EEPROM.commit();
    return feedDatesCount;

  } else {
    return -1;
  }
}

int removefeeding(int index ) {
  Serial.print("removing feeding time: ");
  Serial.println(index);

   int currentFeedIndex=index;
  feedDates[currentFeedIndex] = FeedDate();
  feedDatesCount-=1;
  

  while(currentFeedIndex<MAX_FEEDINGS){
       feedDates[currentFeedIndex]=feedDates[currentFeedIndex+1];
       currentFeedIndex++;       
  }
  
    EEPROM.put(feedingCountAddress, feedDatesCount);
    EEPROM.put(feedingDataAddress, feedDates);
    EEPROM.commit();
    return feedDatesCount;

}






void loop() {
  timeClient.update();
  checkTriggeredFeedTimes();
  //delay(500);

}

void checkTriggeredFeedTimes() {
  time_t epochTime = timeClient.getEpochTime();

  for (int i = 0; i <= feedDatesCount; i++) {
    
    if (epochTime >= feedDates[i].feedDate && !feedDates[i].completed ) {
      Serial.print("Current Epoch Time: ");
      Serial.println(epochTime);

      Serial.printf("Feeding Triggered [%d]: Time: %lu \n", i, feedDates[i].feedDate);
      feedDates[i].completed = true;
      openLid();
      delay(1000);
      closeLid();
      EEPROM.put( feedingDataAddress , feedDates );
      EEPROM.commit();
    }
  }


}

void openLid() {

  servo.attach(0);

  Serial.println("Open Feeder");

  int currentPosition = servo.read();
  Serial.println("Current Pos");
  Serial.println(currentPosition);

  Serial.println("Target Pos");
  Serial.println(configData.openPos);

  for (int pos = currentPosition ; pos >= configData.openPos; pos -= 1) { // goes from 0 degrees to 180 degrees
    Serial.println("-");
    Serial.print("Current Pos");
    Serial.print( pos);
    if (pos == configData.openPos) break;
    // in steps of 1 degree
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(configData.servoSpeed);                       // waits 15ms for the servo to reach the position
  }

  servo.detach();
}


void closeLid() {

  servo.attach(0);
  Serial.println("Close Feeder");

  int currentPosition = servo.read();
  Serial.println("Current Pos");
  Serial.println(currentPosition);

  Serial.println("Target Pos");
  Serial.println(configData.closePos);

  for (int pos = currentPosition ; pos <= configData.closePos; pos += 1) { // goes from 0 degrees to 180 degrees
    Serial.println("+");
    Serial.print("Current Pos");
    Serial.print( servo.read());
    if (pos == configData.closePos) break;
    // in steps of 1 degree
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(configData.servoSpeed);                       // waits 15ms for the servo to reach the position
  }

  servo.detach();
}


void setLidPos(int targetPos) {
  if(targetPos>=0 && targetPos<180 ){
  servo.attach(0);

  Serial.println("Open Feeder");

  int currentPosition = servo.read();
  Serial.println("Current Pos");
  Serial.println(currentPosition);

  Serial.println("Target Pos");
  Serial.println(targetPos);

if(currentPosition>targetPos){
  for (int pos = currentPosition ; pos > targetPos; pos-=1)  { // goes from 0 degrees to 180 degrees
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(configData.servoSpeed);                       // waits 15ms for the servo to reach the position
  }
}else{

 for (int pos = currentPosition ; pos <= targetPos; pos+=1) { 
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(configData.servoSpeed);                       // waits 15ms for the servo to reach the position
  }
}
   servo.detach();
 }
 
}

void sendSMS() {

  Serial.print("Sending SMS");
  HTTPClient http;
  http.begin(smsUrl);
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();

}
