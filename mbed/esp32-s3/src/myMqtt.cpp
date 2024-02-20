#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <process.h>
#include <vector>
#include "myMqtt.h"

using namespace std;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

String _topicNameESP = "/gtsField1/" + String((uint64_t)ESP.getEfuseMac());

char *mqttServer = "broker.hivemq.com";
const char *topicNameESP = _topicNameESP.c_str();

String clientId = "gtsField1-";
int mqttPort = 1883;

mqttRequest MqttRequest;
mqttResponse MqttResponse;

void callback(char *topic, byte *payload, unsigned int length)
{
  static int Led = 1;

  if (*topic == *topicNameESP && PROCESS_FLAG == false)
  {
    MqttRequest.payloadParser(payload, length);

    Led = not Led;

    digitalWrite(2, Led);
  }
  else
  {
    MqttResponse.sendPing(PROCESS_FLAG);
  }
}

void reconnect()
{

  Serial.println("Connecting to MQTT Broker...");

  while (!mqttClient.connected())
  {

    Serial.println("Reconnecting to MQTT Broker..");

    String id = clientId + String(random(0xffff), HEX);

    if (mqttClient.connect(id.c_str()))
    {
      Serial.println("Server Connected.");
      mqttClient.subscribe(topicNameESP);
    }
  }
}

void reconnectTry()
{
  Serial.println("Reconnecting to MQTT Broker..");

  String id = clientId + String(random(0xffff), HEX);

  if (mqttClient.connect(id.c_str()))
  {
    Serial.println("Server Connected.");
    mqttClient.subscribe(topicNameESP);
  }
}

void setupMQTT()
{
  mqttClient.setServer(mqttServer, mqttPort);
  // set the callback function
  mqttClient.setCallback(callback);

  pinMode(2, OUTPUT);
}
