/**
 * This is a kind of unit test for DEV for now
 * It contains many of the public methods
 * 
 */
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <string>
#include <time.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <FS.h>
#include <algorithm>
#include <jsonizer.h>
#include "apMode.h"

TaskHandle_t Task1; // Core 0 task initializer
TaskHandle_t Task2;

void listDir(fs::FS &fs, const char * dirname, uint8_t levels); 
void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);
void loadData(fs::FS &fs, const char * path);
void loop0(void *pvParameters);
void loop1(void *pvParameters);
void reconnect();
std::string callback(char* topic, byte* payload, unsigned int lenght);
String readLicences(fs::FS &fs, const char * path);

WiFiClient espClient; 
apMode apInstance;
PubSubClient client(espClient);
JSONIZER jsonSession;

std::vector<std::string> dataVector;

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
String incommingRequest = "";
std::string licences;
const char* userName;
const char* password;
const int port = 1883;
unsigned long previousTimeMQTT = 0;

void setup(){
  // Dual Core configuration
  // xTaskCreatePinnedToCore: (Loop Function, Task Name, Clock size, function parameter, Core priority, Task instance, core where loop will be executed)
  // Serial config
  Serial.begin(115000);
  //File System and configuration setup
  if(!LittleFS.begin()){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  listDir(LittleFS, "/", 0);
  readFile(LittleFS, "/config.json");
  //readFile(LittleFS, "/licences.txt");
  loadData(LittleFS, "/config.json");
  //AP setup.toSJSON()
  apInstance.setupServer(staticIpAP, gatewayAP, subnetMaskAP);

  //MQTT
  client.setServer(host.c_str(), port);
  client.setCallback(callback); //Callback 
  xTaskCreatePinnedToCore(loop0, "core_0", 10000, NULL, 1, &Task1, 0);
  xTaskCreatePinnedToCore(loop1, "core_1", 10000, NULL, 1, &Task2, 1);
}

void loop(){}

void loop1(void *pvParameters){
  for(;;){
    std::string cores = String(ESP.getChipCores()).c_str();
    std::string frecuency = String(ESP.getCpuFreqMHz()).c_str();
    std::string sketchSize = String(ESP.getSketchSize()).c_str();
    std::string freeHeap = String(ESP.getFreeHeap()).c_str();
    std::string freeRam = String(ESP.getFreePsram()).c_str();
    dataVector.push_back("Server");dataVector.push_back("Giuli-Licence-Server");dataVector.push_back("Cores");dataVector.push_back(cores);
    dataVector.push_back("FreeRam");dataVector.push_back(freeRam);dataVector.push_back("CPUfrecuency");dataVector.push_back(frecuency);
    dataVector.push_back("SketchSize");dataVector.push_back(sketchSize);dataVector.push_back("FreeHeap");dataVector.push_back(freeHeap);   
    
    if(!client.connected()){
      reconnect();
    }

    //Serial.println(String(xPortGetCoreID()));
    if(client.connected()){
      Serial.println(jsonSession.toSJSON(dataVector).c_str());
      client.publish(root_topic_publish.c_str(), jsonSession.toSJSON(dataVector).c_str());
      delay(1000);
    }
    
    dataVector.clear();
    client.loop();
  }
}

void loop0(void *pvParameters){
  for(;;){
    
    if(incommingRequest != "" ){
      std::vector<std::string> request = jsonSession.toVECTOR(incommingRequest.c_str());
      Serial.println(request.at(3).c_str());
      delay(10000);
    }
  }
  vTaskDelay(10);
}

std::string callback(char* topic, byte* payload, unsigned int lenght){
  String incomingMessage = "";
  Serial.print("desde > ");
  Serial.print(topic);
  Serial.println("");
  for(int n = 0; n < lenght; n++){
    incomingMessage += (char)payload[n];
  }
  incomingMessage.trim();

  incommingRequest = incomingMessage;
 
  /*
  std::vector<std::string> request = jsonSession.toVECTOR(incomingMessage.c_str());
  std::vector<std::string> sendOK, sendBAD;
  sendOK.push_back("status");sendOK.push_back("true");
  sendBAD.push_back("status");sendBAD.push_back("false");
  
  if(String(jsonSession.readFileIntoString("/licences.txt").c_str()).indexOf(request.at(3).c_str()) > 0){
    Serial.println(jsonSession.toSJSON(sendOK).c_str());
    client.publish(root_topic_publish.c_str(), jsonSession.toSJSON(sendOK).c_str());   
  }else{Serial.println(jsonSession.toSJSON(sendBAD).c_str());client.publish(root_topic_publish.c_str(), jsonSession.toSJSON(sendBAD).c_str());}
  
  request.clear();sendOK.clear();sendBAD.clear();
  */
  return incomingMessage.c_str();
}


void reconnect(){
  int count = 0;
  while(!client.connected()){
    String deviceId = String("Server");
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

    File root = fs.open(dirname);
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

    File file = fs.open(path);
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

String readLicences(fs::FS &fs, const char * path){
    std::ifstream inFile;
    inFile.open(path); //open the input file

    std::stringstream strStream;
    strStream << inFile.rdbuf(); //read the file
    std::string str = strStream.str();

    return String(str.c_str());
}

void loadData(fs::FS &fs, const char * path){
    File file_ = fs.open(path);
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

    file = fs.open(path);
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
