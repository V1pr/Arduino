Simple Arduino sketch for ESP8266 to work with an attached relay(s) and a DHT11, reporting to MQTT server.

Based on Erick Joaquin's work: https://community.home-assistant.io/t/arduino-relay-switches-over-wifi-controlled-via-hass-mqtt-comes-with-ota-support/13439/41

To get things work:
- buy hardwares, connect them together
- Install Arduino IDE
- Install board support for ESP8266 ( add URL to preferences, Tools -> Board -> Board manager )
- Install needed libraries ( PubSub, AdafruidSensor, DHT, ArduinoOTA, etc)
- Compile and use :)

I've tested this with HomeAssistant and Mosquitto.

HASS:

sensor 2:
  platform: mqtt
  name: "Temperature"
  state_topic: "sensor/temperature"
  qos: 0
  unit_of_measurement: "oC"

sensor 3:
  platform: mqtt
  name: "Humidity"
  state_topic: "sensor/humidity"
  qos: 0
  unit_of_measurement: "%"

switch 1:
  - platform: mqtt
    name: "MQTT Switch 1"
    state_topic: "house/switch1/Confirm"
    command_topic: "house/switch1"
    payload_on: 1
    payload_off: 0
    qos: 0
    retain: true
