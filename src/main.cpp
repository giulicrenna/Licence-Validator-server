/**
 * This is a kind of unit test for DEV for now
 * It contains many of the public methods
 * 
 */
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <string>
#include <time.h>
#include <stdio.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include "jsonizer.h"
#include "apMode.h"

void listDir(fs::FS &fs, const char * dirname, uint8_t levels); 
void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);
void loadData(fs::FS &fs, const char * path);
void callback(char* topic, byte* payload, unsigned int lenght);

WiFiClient espClient; 
apMode apInstance;
PubSubClient client(espClient);
JSONIZER jsonSession;

String deviceName;
String staticIpAP;
String gatewayAP;
String subnetMaskAP;
String host;
String root_topic_subscribe;
String root_topic_publish;
String smtpSender;
String smtpPass;
String SmtpReceiver;
String SmtpServer;
const char* userName;
const char* password;
const int port = 1883;
unsigned long previousTimeMQTT = 0;

void setup(){
  Serial.begin(115000);

  delay(2000);


  //File System and configuration setup
  if(!LittleFS.begin()){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  listDir(LittleFS, "/", 0);
  readFile(LittleFS, "/config.json");
  loadData(LittleFS, "/config.json"); 
  //AP setup
  apInstance.setupServer(staticIpAP, gatewayAP, subnetMaskAP);

  //MQTT
  client.setServer(host.c_str(), port);
  client.setCallback(callback); //Callback 
}


void loop(){
  if(!client.connected()){
    reconnect();
  }

  std::string licences = jsonSession.readFileIntoString("licences.json");

  if(client.connected()){
    unsigned long currentTime = millis();
    if(currentTime - previousTimeMQTT >= 500){
      //std::string jsonData = jsonSession.toSJSON(dataVector);
      Serial.println(licences.c_str());
      client.publish(root_topic_publish.c_str(), licences.c_str());
      previousTimeMQTT = currentTime;
    }
  }
  client.loop();
}


void callback(char* topic, byte* payload, unsigned int lenght){
  String incomingMessage = "";
  Serial.print("desde > ");
  Serial.print(topic);
  Serial.println("");
  for(int n = 0; n < lenght; n++){
    incomingMessage += (char)payload[n];
  }
  incomingMessage.trim();
  Serial.println(" >>" + incomingMessage);
}


void reconnect(){
  int count = 0;
  while(!client.connected()){
    String deviceId = String("DarkFlow_" + ESP.getChipId());
    String message = "Intentando conectar a: " + String(host) + ", Con ID: " + deviceId; 
    if(client.connect(deviceId.c_str())){
      Serial.println("Conexión Exitosa");
      if(client.subscribe(root_topic_subscribe.c_str())){
        Serial.println("Subscripción exitosa");
      }else{
        Serial.println("subscripción Fallida...");
      }
    }else{
      count += 1;
      Serial.print("Falló la conexión / Error >");
      Serial.println(client.state());
      Serial.println("Intentando nuevamente en 10 Segundos");
      delay(10000);
    }
  }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname, "r");
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path, "r");
    if(!file || file.isDirectory()){
        Serial.println("...failed to open file for reading");
        return;
    }

    Serial.println("...read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    Serial.println("");

    file.close();   
}

void loadData(fs::FS &fs, const char * path){
    File file_ = fs.open(path, "r");
    String content;
    if(!file_.available()){
      Serial.println("Couldn't open the file");  
    }
    while (file_.available()) {
      content += file_.readString();
      break;
    }
    StaticJsonDocument<1024> config;
    auto error = deserializeJson(config, content);

    if(error){
      Serial.println("Failed to deserialize");
      Serial.println(error.f_str());
    }
    
    //Serial.println(config.size());

    deviceName = (const char*)config["device"]["name"];
    staticIpAP = (const char*)config["network"]["ip"];
    subnetMaskAP = (const char*)config["network"]["subnetMask"];
    gatewayAP = (const char*)config["network"]["gateway"];
    host = (const char*)config["mqtt"]["host"];
    root_topic_subscribe = (const char*)config["mqtt"]["root_topic_subscribe"];
    root_topic_publish = (const char*)config["mqtt"]["root_topic_publish"];
    smtpSender = (const char*)config["smtp"]["mailSender"];
    smtpPass = (const char*)config["smtp"]["mailPassword"];
    SmtpReceiver = (const char*)config["smtp"]["mailReceiver"];
    SmtpServer = (const char*)config["smtp"]["smtpServer"];

    Serial.println("#### CONFIG LOADED ####");

    file_.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, "w");
    if(!file){
        Serial.println("...failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("...file written");
    } else {
        Serial.println("...write failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("...file renamed");
    } else {
        Serial.println("...rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("...file deleted");
    } else {
        Serial.println("...delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, "w");
    if(!file){
        Serial.println("...failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("...writing" );
    uint32_t start = millis();
    for(i=0; i<2048; i++){
        if ((i & 0x001F) == 0x001F){
          Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path, "r");
    start = millis();
    end = start;
    i = 0;
    if(file && !file.isDirectory()){
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("...reading" );
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F){
              Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    } else {
        Serial.println("...failed to open file for reading");
    }
}
