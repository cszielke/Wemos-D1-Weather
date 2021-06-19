//https://medium.com/marcel-works/esp8266-zur-laufzeit-konfigurieren-3b9dfcb5dfac
#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>

#include <Ethernet.h>
#define NO_OTA_PORT
#include <ArduinoOTA.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "ArduinoJson.h"
#include "WiFiManager.h"
#include "PubSubClient.h"

#include "weather.h"

#define CLEAR_CONFIG false
#define roottopic "Wetterstation"
#define FWVERSION "V1.0.0"

bool shouldSaveConfig  = false;
const int CHIP_ID = ESP.getChipId();

//Output
//const int REL_OPEN_DOOR = D5; //GPIO14

//const int SW_DOOR = D6; // GPIO12
//const int SW_RESERVED = D7; //GPIO13

// I2C
const int I2C_CLK = D1; //GPIO05
const int I2C_DAT = D2; //GPIO04

//Ultrasonic distance
//const int TRIGGER = D4; //GPIO02 (High on Boot, boot failture if pulled LOW)
//const int ECHO = D0; //GPIO16 (High on Boot)

//long duration;
//int distance;

uint32_t lasttime;

//MQTT
char mqtt_server[40] = "192.168.15.107";
char mqtt_port[6] = "1883";
char mqtt_user[40] = "user";
char mqtt_pass[40] = "password";

char mqtt_root_topic[34] = roottopic;
char mqtt_wifirssi_topic[44];
char mqtt_battery_topic[44];

char mqtt_sub_cmd_topic[44];

float mqtt_wifirssi;
float mqtt_battery;

float last_val_wifirssi;
float last_val_battery;

int counter = 0; //Update MQTT from time to time...
#define MaxSendInterval 30

Weather weather;

WiFiClient espClient;
PubSubClient client(espClient);

char tmp[21];

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig  = true;
}

void SubCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == mqtt_sub_cmd_topic) { //"Wetterstation/14506/202/cmd"){ 
    Serial.print("Command received: ");
    Serial.println("messageTemp");
  }
}

void setup() {
  //pinMode(SW_RESERVED, INPUT);
  //pinMode(SW_DOOR, INPUT);
  
  //pinMode(REL_OPEN_DOOR, OUTPUT);
  //digitalWrite(REL_OPEN_DOOR,0);
  
  //pinMode(TRIGGER, OUTPUT); // Sets the trigPin as an Output
  //pinMode(ECHO, INPUT); // Sets the echoPin as an Input

  Serial.begin(115200);
  Serial.println();
  Serial.print("chip id: ");
  Serial.println(CHIP_ID);
  Serial.print("Firmware Version: ");
  Serial.println(FWVERSION);

  strcat(mqtt_root_topic, "/");
  strcat(mqtt_root_topic,  String(CHIP_ID).c_str());

  snprintf(mqtt_wifirssi_topic,sizeof(mqtt_wifirssi_topic),"%s/%s",mqtt_root_topic, "wifirssi");
  snprintf(mqtt_battery_topic,sizeof(mqtt_battery_topic),"%s/%s",mqtt_root_topic, "battery");
  
  snprintf(mqtt_sub_cmd_topic,sizeof(mqtt_sub_cmd_topic),"%s/%s",mqtt_root_topic, "cmd");
  
  Serial.print("Subscribing to Topic:");
  Serial.println(mqtt_sub_cmd_topic);
  
  //clean FS, for testing
  if(CLEAR_CONFIG) LittleFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");
  if (LittleFS.begin()) {
    Serial.println("mounted file system");
      if (LittleFS.exists("/config.json")) {
        //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(1024);
        DeserializationError error = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if (!error) {
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_pass, json["mqtt_pass"]);
          strcpy(mqtt_root_topic, json["mqtt_root_topic"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount file system");
  }

  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 40);
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, 40);
  WiFiManagerParameter custom_mqtt_root_topic("topic", "mqtt root topic", mqtt_root_topic, 34);

  WiFiManager wifiManager;  

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_mqtt_root_topic);

  //reset settings - for testing
  if(CLEAR_CONFIG) wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(120);


  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "NodeMCU-AP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("NodeMCU-AP", "12345678")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  Serial.println("connected");
  
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  strcpy(mqtt_root_topic, custom_mqtt_root_topic.getValue());

  if (shouldSaveConfig ) {
    Serial.println("saving config");

    DynamicJsonDocument json(1024);
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_pass"] = mqtt_pass;
    json["mqtt_root_topic"] = mqtt_root_topic;

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
  }

  Serial.println("assigned ip");
  Serial.println(WiFi.localIP());
  Serial.println("Signal strength");
  Serial.println(WiFi.RSSI());

  client.setServer(mqtt_server, 1883);
  client.setCallback(SubCallback);

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"rovema");

  ArduinoOTA.onStart([]() {
    Serial.print(F("Start OTA"));
  });
  ArduinoOTA.onEnd([]() {
    Serial.print(F("End OTA"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA-Progress: %u%%\r", (progress / (total / 100)));

  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

    // start the OTEthernet library with internal (flash) based storage
  ArduinoOTA.begin();
  lasttime = millis();
  weather.Init(client,Serial,mqtt_root_topic);
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("trying to connect to mqtt...");
    if (client.connect("ESP8266Client_WS")) {
      Serial.println("connected");

      client.subscribe(mqtt_sub_cmd_topic);

    } else {
      Serial.print("connecting to mqtt broker failed, rc: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

int32_t publish_WifiRssi(void)
{
  int32_t sig = WiFi.RSSI();
  snprintf(tmp,sizeof(tmp),"%d",sig);
  client.publish(mqtt_wifirssi_topic, tmp);

  Serial.print("Signal strength: ");
  Serial.println(WiFi.RSSI());

  return sig;
}

float publish_Battery(void)
{
  float aval = analogRead(A0);
  for(int i=0; i<5;i++)
  {
    aval += analogRead(A0);
    delay(11);
  }
  aval = aval/6;

  float batlevel = 4.25F/1024*aval;
  snprintf(tmp,sizeof(tmp),"%3.2f",batlevel);
  client.publish(mqtt_battery_topic, tmp);

  Serial.print("Battery level: ");
  Serial.println(tmp);

  return batlevel;
}

void loop() {  
  //weather.loop();
  
  uint32_t now = millis();
  if((lasttime + 1000) < now)
  {
    lasttime = now;
  
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  
    //Read Inputs
    weather.GetValues();

    //Send Values
    weather.PrintValues();

    //If at least 1 Value was send, Publish also Bat and Rssi
    if(weather.PublishValues(counter >= MaxSendInterval) > 0)
    {
      publish_WifiRssi();
      publish_Battery();
    }
    
    counter++;
    if(counter >= (MaxSendInterval + 1)){
      counter = 0;
    }
    ArduinoOTA.handle();
    delay(100);
  }
}