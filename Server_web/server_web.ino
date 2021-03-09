// Load library
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <FS.h>
#include <stdlib.h>
#include <ArduinoJson.h>
#include "AsyncJson.h"
#include <TimeLib.h>
#include <TimeAlarms.h>

// Replace with your network credentials
const char* ssid = "BELL598"; //dlink-34FA
const char* password = "4D11E39D7A6F"; // d5e8cb5674

//time set
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000;
const int   daylightOffset_sec = 3600;

// Auxiliar variables to store the current output state
String lightState;

// alarm variables
bool alarmIsOn;
String alarmTime;
int alarmHour;
int alarmMin;
AlarmID_t alarmID;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Writes on a file and serial print the message
void writeState(const String& path, const String& txt){
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("There was an error opening the file for writing");
    return;
  }
  if(file.print(txt)){
    Serial.println("File was written");;
  } else {
    Serial.println("File write failed");
  }
  file.close();
  
  File f = SPIFFS.open(path);
  if(!f){
      Serial.println("Failed to open file for reading");
      return;
  }
  Serial.println("File Content:");
  while(f.available()){
      Serial.write(f.read());
  }
  Serial.println(' ');
  f.close();
}

// Read file return result
String readState(const String& path){
  File f = SPIFFS.open(path);
  if(!f || !f.size()){
      //Serial.println("Failed to open file for reading");
      return "Failed to open file for reading";
  }
  String msg = "";
  while(f.available()){
      msg += char(f.read());
  }
  //Serial.println("read:");
  //Serial.println(msg);
  f.close();
  return msg;
}

void applyState(const String& path){
  if (readState(path) == "7978" || readState(path) == "ON"){
    digitalWrite(5, 1);
  }
  else if (readState(path) == "797070" || readState(path) == "OFF"){
    digitalWrite(5, 0);
  }  
}

void changeState(const String& path){
  Serial.println("State => ");
  Serial.println(lightState);
  if(lightState == "OFF"){
    writeState(path, "OFF");
  }
  else if(lightState == "ON"){
    writeState(path, "ON");
  }
  applyState(path); 
}

// Replaces placeholder with light state value
/*String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(5)){
      outputState = "ON";
    }
    else{
      outputState = "OFF";
    }
    return outputState;
  }
  return String();
}*/

void alarm(){
  writeState("/state.txt", "ON");
  applyState("/state.txt");
  Alarm.disable(alarmID);
  Alarm.free(alarmID);
}

void alarmHandle(){
  char buffer[64];
  alarmTime.toCharArray(buffer, sizeof(buffer));
  char* token = strtok(buffer,":");
  char* bufferHour;
  char* bufferMin;
  int i = 0;
  while( token != NULL ) {
    printf( " %s\n", token ); //printing each token
    if(i == 0){bufferHour = token;}
    if(i == 1){bufferMin = token;}
    token = strtok(NULL, ":");
    i += 1;
 }
  Serial.println("alarm => ");
  Serial.println(alarmTime);
  alarmHour = (int) strtol(bufferHour, (char **)NULL, 10);
  alarmMin = (int) strtol(bufferMin, (char **)NULL, 10); //error here
  Serial.println(alarmHour);
  Serial.println(alarmMin);

  if(alarmID){
    Alarm.disable(alarmID);
    Alarm.free(alarmID);
  }
  alarmID = Alarm.alarmRepeat(alarmHour, alarmMin, 0 , alarm);
  if(alarmIsOn == false){
    Alarm.disable(alarmID);
    Alarm.free(alarmID);
  }  
}

//time display
void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println(); 
}
void printDigits(int digits)
{
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setup() {
  Serial.begin(9600);
  // Initialize the output variables as outputs
  pinMode(5, OUTPUT);
  
  // Set outputs to hight then LOW dummy
  digitalWrite(5, 1);
  digitalWrite(5, 0);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  String webIP= (WiFi.localIP()).toString();
  Serial.println(webIP);
  
  //init time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  time_t t = mktime(&timeinfo);
  setTime(t);
  adjustTime(-18000);
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route alarm get
  /*server.on("/alarm", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/alarm.txt", String(), false);
    alarmTime = readState("/alarm.txt");
    Serial.println(alarmTime);
  });
  
  // Route alarm post
  server.on("/alarm", HTTP_POST, [](AsyncWebServerRequest * request){},
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    String msg;
    for (size_t i = 0; i < len; i++) {
      msg += char(data[i]);
    }
    
    alarmTime = msg;
    char buffer[64];
    alarmTime.toCharArray(buffer, sizeof(buffer));
    char* token = strtok(buffer,":");
    char* bufferHour;
    char* bufferMin;
    int i = 0;
    while( token != NULL ) {
      printf( " %s\n", token ); //printing each token
      if(i == 0){bufferHour = token;}
      if(i == 1){bufferMin = token;}
      token = strtok(NULL, ":");
      i += 1;
   }
    Serial.println("alarm => ");
    Serial.println(alarmTime);
    alarmHour = (int) strtol(bufferHour, (char **)NULL, 10);
    alarmMin = (int) strtol(bufferMin, (char **)NULL, 10); //error here
    Serial.println(alarmHour);
    Serial.println(alarmMin);

    if(alarmID){
      Alarm.disable(alarmID);
      Alarm.free(alarmID);
    }    
    alarmID = Alarm.alarmRepeat(alarmHour, alarmMin, 0 , alarm);
    
    request->send(200);
  });*/

  // data route get
  server.on("/get-data", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<100> data;
    if (request->hasParam("state"))
    {
      data["state"] = request->getParam("state")->value();
    }
    else {
      data["state"] = "No message parameter";
    }
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
  });

  // data route post
  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/post-data", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }
    String response;
    serializeJson(data, response);
    Serial.println(response);
    String state = data["state"];
    lightState = state;
    changeState("/state.txt");
    String tmp = data["alarmTime"];
    alarmTime = tmp;
    Serial.println("alameTime ==> " + alarmTime);
    alarmIsOn = data["alarm"];
    Serial.println(alarmIsOn);
    alarmHandle();
    
    request->send(200, "application/json", response);
  });
  server.addHandler(handler);

  // Route state get
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/state.txt", String(), false);
    applyState("/state.txt");
  });
  

  // Route state post
  server.on("/state", HTTP_POST, [](AsyncWebServerRequest * request){},
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    String msg;
    for (size_t i = 0; i < len; i++) {
      msg += char(data[i]);
    }
    Serial.println("repState => ");
    Serial.println(msg);
    if(msg == "OFF"){
      writeState("/state.txt", "OFF");
    }
    else if(msg == "ON"){
      writeState("/state.txt", "ON");
    }
    applyState("/state.txt");
    request->send(200);
  });
  
  // Route to set GPIO to HIGH web
  /*server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    //digitalWrite(5, HIGH);
    //writeState("/state.txt", "ON");    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to set GPIO to LOW web
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    //digitalWrite(5, LOW); 
    //("/state.txt", "OFF");   
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });*/

  // start server
  server.begin();
}

void loop(){
  digitalClockDisplay();
  Alarm.delay(1000); // wait one second between clock display
  applyState("/state.txt");
}
