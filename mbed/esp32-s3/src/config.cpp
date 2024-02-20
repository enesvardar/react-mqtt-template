#include <Arduino.h>
#include "EEPROM.h"
#include <myWifi.h>
#include <myMqtt.h>
#include <email.h>
#include "access_point.h"
#include "eprom.h"
#include "process.h"
#include "config.h"
#include <sstream>
#include <iostream>

using namespace std;

void esp32Init(void)
{
  pinMode(1, INPUT); // BTN

  initEprom();

  delay(1000);

  bool axcessPoint = false;

  if (digitalRead(1) == 0)
  {
    delay(2000);

    if (digitalRead(1) == 0)
    {
      axcessPoint = true;
    }
  }

  int ssidLen = EEPROM.read(0);
  int passLen = EEPROM.read(1);

  Serial.println("axcessPoint");
  Serial.println(axcessPoint);

  axcessPoint = false;

  if (ssidLen > 0 && ssidLen < 50 && passLen > 0 && passLen < 50 && axcessPoint == false)
  {

    String ssid = readEpromString(2, ssidLen);
    String pass = readEpromString(3 + ssidLen, passLen);

    if (!connectToWiFi(ssid.c_str(), pass.c_str()))
    {
      esp_restart();
    }
  }
  else
  {
    Serial.println("accesPointInit");
    accesPointInit();
    accesPointLoop();
    sendEmailMac();
  }

  setupMQTT();

  delay(2000);
}