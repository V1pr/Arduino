// multiple Relay Switch over WIFI using MQTT and Home Assistant with OTA Support 
// written by Tamas DAJKA, Hungary (V1pr@GitHub)
//
// Based on Erick Joaquin's work: https://community.home-assistant.io/t/arduino-relay-switches-over-wifi-controlled-via-hass-mqtt-comes-with-ota-support/13439/41
//EJ: The only way I know how to contribute to the HASS Community... you guys ROCK!!!!!
//EJ: Erick Joaquin :-) Australia

// Original sketch courtesy of ItKindaWorks
//ItKindaWorks - Creative Commons 2016
//github.com/ItKindaWorks
//
// the sketch below works with a GeekCreit ESP12F V3 ( nodeMCU clone)
//
//Required libraries - Arduino IDE -> Sketch -> Libraries -> Managge:
// * PubSubClient ( found also here: https://github.com/knolleary/pubsubclient )
// * ArduinoOTA
// * DHT
// * Adafruit sensors
//
//ESP8266 Simple MQTT switch controller

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <DHT.h>
#include <ctype.h>

void callback(char* topic, byte* payload, unsigned int length);

//EDIT THESE LINES TO MATCH YOUR SETUP
const char* ssid = "your_ssid";
const char* password = "your_wifi_password";

#define mqtt_server "your_mqtt_server_ip"
#define mqtt_user "your_mqtt_server_user"
#define mqtt_password "your_mqtt_server_pass"

#define humidity_topic "sensor/humidity"
#define temperature_topic "sensor/temperature"

#define DHTTYPE DHT11
// pin 14 is D5 for my board
#define DHTPIN  14


//EJ: Data PIN Assignment on WEMOS D1 R2 https://www.wemos.cc/product/d1.html
// if you are using Arduino UNO, you will need to change the "D1 ~ D4" with the corresponding UNO DATA pin number 
//
// V1pr: add pins with relays to array switchPins and set switchCnt to the number relays attached (max. 9)
//

// max 9 switches!
// help for pinout:
// D4 = 2
// D2 = 4
int switchPins[] = { 4 };
/*
 * Type of the switch
 * 
 *   0: on/off
 *   >0: will set switch ON for this DELAY miliseconds - toggle
 */
int switchTypes[] = { 150 };

// automatic numbering and trailing slash - this is the MQTT base address - modify to suit you setup, or leave it as it is :)
const char* switchTopic_base = "house/switch";

// sleep time - set to 0 to disable
const int sleepTimeS = 3;


/*
 * 
 *  NO MODIFICATION NEEDED BELOW
 * 
 */

WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, callback, wifiClient);
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

int switchCnt = 1;
long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float diff = 1.0;


void setup() {

  //start the serial line for debugging
  Serial.begin(115200);
  Serial.println();
  delay(150);

  // calculate the array length
  switchCnt = sizeof(switchPins)/sizeof(int);
  
  //initialize the switch pins as an output and set to LOW (off)
  for ( int idx = 0; idx < switchCnt; idx++ ) {
    Serial.print("Initializing RELAY ");
    Serial.print(idx+1);
    Serial.print(" on GPIO pin ");
    Serial.println(switchPins[idx]);
    pinMode(switchPins[idx], OUTPUT); // Relay Switch 1
    digitalWrite(switchPins[idx], LOW);
  }

  ArduinoOTA.setHostname("Arduino ESP"); // A name given to your ESP8266 module when discovering it as a port in ARDUINO IDE
  ArduinoOTA.begin(); // OTA initialization

  //start wifi subsystem
  WiFi.begin(ssid, password);
  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

  Serial.println("Setup OK");
  
  //wait a bit before starting the main loop
  delay(500);
}


void loop(){

  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {reconnect();}

  //maintain MQTT connection
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();

    if (checkBound(newTemp, temp, diff)) {
      temp = newTemp;
      Serial.print(now);
      Serial.print(" New temperature: ");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic, String(temp).c_str(), true);
    }

    if (checkBound(newHum, hum, diff)) {
      hum = newHum;
      Serial.print(now);
      Serial.print(" New humidity:");
      Serial.println(String(hum).c_str());
      client.publish(humidity_topic, String(hum).c_str(), true);
    }
  }

  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10); 
  ArduinoOTA.handle();

  // Sleep
  if ( sleepTimeS > 0 ) {
    Serial.println("ESP8266 in sleep mode");
    ESP.deepSleep(sleepTimeS * 1000000);
  }
}

/*
 * 
 * Callback to handle MQTT server calls
 */
void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic; 

  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);
  Serial.print("Payload: ");
  char *buffer = (char *)payload;
  Serial.println(buffer[0]);
  if ( topicStr.indexOf('switch') != -1 ) {
    
    //char switchNum = topicStr.charAt(topic[strlen(topic)-1]);
    char switchNum = topic[strlen(topic)-1];
    int swNum = switchNum - '0';
    
    //EJ: Note:  the "topic" value gets overwritten everytime it receives confirmation (callback) message from MQTT
    //Serial.print("SwitchNum: ");
    //Serial.println(switchNum);
    //Serial.println(swNum);

    char swTopicConf[strlen(switchTopic_base)+10];
    sprintf(swTopicConf,"%s%d/Confirm",switchTopic_base,swNum);
    Serial.println(swTopicConf);

    if ( swNum <= switchCnt ) {
      //turn the switch on if the payload is '1' and publish to the MQTT server a confirmation message
      Serial.print("Changing switch ");
      Serial.print(swNum);
      Serial.print(" to state: ");
      Serial.println(buffer[0]);
      if ( buffer[0] == '1' ) {
         digitalWrite(switchPins[swNum-1], HIGH);
         client.publish(swTopicConf, "1");
         if ( switchTypes[swNum-1] > 0 && isdigit(switchTypes[swNum-1]) ) {
            delay(switchTypes[swNum-1]);
            digitalWrite(switchPins[swNum-1], LOW);
            client.publish(swTopicConf, "0");
         }
      } else if (buffer[0] == '0' ){
         digitalWrite(switchPins[swNum-1], LOW);
         client.publish(swTopicConf, "0");
      }
    }
  } else {
    Serial.println("Unknown topic: "+topicStr); 
  }
}


void reconnect() {

  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){
    //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    //print out some more debug once connected
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
    delay(10);
   // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);

      //if connected, subscribe to the topic(s) we want to be notified about
      //EJ: Delete "mqtt_username", and "mqtt_password" here if you are not using any 
      if (client.connect((char*) clientName.c_str(),"mqtt_username", "mqtt_password")) {  //EJ: Update accordingly with your MQTT account 
        Serial.println("\tMQTT Connected - subscribing to topics");
        //Serial.println(switchTopic_base);
        //Serial.print("Size: ");
        //Serial.println(strlen(switchTopic_base));
        char swT[strlen(switchTopic_base)+3];
        for (int idx = 1; idx <= switchCnt; idx++) {
          sprintf(swT,"%s%d",switchTopic_base,idx);
          Serial.println(swT);
          client.subscribe(swT);
        }
      }
      //otherwise print failed for debugging
      else{Serial.println("\tFailed."); abort();}
      delay(50);
    }
  }
  Serial.println("Wifi OK");
}

//generate unique name from MAC addr
String macToStr(const uint8_t* mac){

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }

  return result;
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}
