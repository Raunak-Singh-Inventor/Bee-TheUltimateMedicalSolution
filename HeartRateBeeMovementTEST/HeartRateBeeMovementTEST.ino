#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000

#include "arduino_secrets.h"

/////// Enter your sensitive data in arduino_secrets.h
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
const char* certificate  = SECRET_CERTIFICATE;

WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

unsigned long lastMillis = 0;

#include <Wire.h>

#include "Grove_Human_Presence_Sensor.h"

AK9753 movementSensor;

// need to adjust these sensitivities lower if you want to detect more far
// but will introduce error detection
float sensitivity_presence = 6.0;
float sensitivity_movement = 10.0;
int detect_interval = 30; //milliseconds
PresenceDetector detector(movementSensor, sensitivity_presence, sensitivity_movement, detect_interval);

uint32_t last_time;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  // Set a callback to get the current time
  // used to validate the servers certificate
  ArduinoBearSSL.onGetTime(getTime);

  // Set the ECCX08 slot to use for the private key
  // and the accompanying public certificate for it
  sslClient.setEccSlot(0, certificate);

  // Optional, set the client id used for MQTT,
  // each device that is connected to the broker
  // must have a unique client id. The MQTTClient will generate
  // a client id for you based on the millis() value if not set
  //
  // mqttClient.setId("clientId");

  // Set the message callback, this function is
  // called when the MQTTClient receives a message
  mqttClient.onMessage(onMessageReceived);

  Wire.begin();

  //Turn on sensor
  if (movementSensor.initialize() == false) {
      Serial.println("Device not found. Check wiring.");
      while (1);
  }

  last_time = millis();  
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }

  // poll for new MQTT messages and send keep alives
  mqttClient.poll();

  detector.loop();
  uint32_t now = millis();
  if (now - last_time > 100) {
      #if 0
      
      //see the derivative of a specific channel when you're adjusting the threshold
      //open the Serial Plotter
       mqttClient.beginMessage("arduino/outgoing");

      mqttClient.print(detector.getDerivativeOfIR1());
      mqttClient.print(" ");
      mqttClient.print(detector.getDerivativeOfIR2());
      mqttClient.print(" ");
      mqttClient.print(detector.getDerivativeOfIR3());
      mqttClient.print(" ");
      mqttClient.println(detector.getDerivativeOfIR4());
      mqttClient.endMessage();
}
      #else
      if (detector.presentFullField(false)) {
          mqttClient.beginMessage("arduino/outgoing");
          mqttClient.print(detector.presentField1() ? "x " : "o ");
          mqttClient.print(detector.presentField2() ? "x " : "o ");
          mqttClient.print(detector.presentField3() ? "x " : "o ");
          mqttClient.print(detector.presentField4() ? "x " : "o ");
          mqttClient.print(" millis: ");
          mqttClient.println(now);
          mqttClient.endMessage();
      }
      #endif
      last_time = now;
  }

  delay(1);
}

unsigned long getTime() {
  // get the current time from the WiFi module  
  return WiFi.getTime();
}

void connectWiFi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print(" ");

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the network");
  Serial.println();
}

void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");

  while (!mqttClient.connect(broker, 8883)) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe("arduino/incoming");
}

void publishMessage() {
  Serial.println("Publishing message");

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("arduino/outgoing");
  mqttClient.print("hello ");
  mqttClient.print(millis());
  mqttClient.endMessage();
}

void onMessageReceived(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    Serial.print((char)mqttClient.read());
  }
  Serial.println();

  Serial.println();
}
