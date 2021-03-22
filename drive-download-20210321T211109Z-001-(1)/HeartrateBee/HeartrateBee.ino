#include <Wire.h>
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


void setup() {
    Serial.begin(9600);
    while (!Serial);

    if (!ECCX08.begin()) {
      Serial.println("No ECCX08 present!");
      while (1);
    }
    
    Serial.println("heart rate bee");
    Wire.begin();

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
}// end setup

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
    Serial.print("wifi retry.....");
    delay(500);
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
    Serial.print("mqtt retry.....");
    delay(500);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe("heartratebee/incoming");
  
}


/*
 * Receive message code
 */
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
/*
 * Measure heart rate
 */
int heartRate(){
    // Serial.println("Starting to measure heart rate");  
    Wire.requestFrom(0xA0 >> 1, 1);    // request 2 bytes from slave device
    while(Wire.available()) {          // slave may send less than requested
       int c = Wire.read();   // receive heart rate value (a byte)
        // Serial.println(c, DEC);         // print heart rate value
        return c;
    }
   // delay(500);
   return 1000;
 }
/*
 * PUBLISH MEssage code
 */
void publishMessage(int input) {
    if (!mqttClient.connected()) {
      // MQTT client is disconnected, connect
      connectMQTT();
    }
  // Serial.println("Publishing message");
  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("heartratebee/outgoing");
  mqttClient.print(input);
  mqttClient.endMessage();

}

void loop() {
   Wire.begin();
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }
    
//     // poll for new MQTT messages and send keep alives
//    mqttClient.poll();
    int c = heartRate();
    Serial.println(c, DEC);
    delay(5000);
 
    publishMessage(c);

}
