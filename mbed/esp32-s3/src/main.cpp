#include <myWifi.h>
#include <myMqtt.h>
#include "config.h"
#include <process.h>
#include "WiFi.h"

bool sendPing = false;

void setup()
{
  Serial.begin(115200);
  esp32Init();
  Serial.println("ESP32_" + String((uint64_t)ESP.getEfuseMac()));
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!mqttClient.connected())
    {
      reconnect();
      sendPing = false;
    }
    else
    {
      if (sendPing == false)
      {
        sendPing = true;
        MqttResponse.sendPing(PROCESS_FLAG);
      }

      mqttClient.loop();
      processLoop();
    }
  }
  else
  {
    WiFi.disconnect();
    WiFi.reconnect();

    while (!connectToWiFiAgain())
      ;
  }
}
