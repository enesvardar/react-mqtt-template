#include <Arduino.h>
#include <WiFi.h>

String SSID;
String PWD;

bool connectToWiFi(const char *_SSID, const char *_PWD)
{
  // Set your Static IP address
  IPAddress local_IP(192, 168, 1, 184);
  // Set your Gateway IP address
  IPAddress gateway(192, 168, 1, 1);

  IPAddress subnet(255, 255, 0, 0);
  IPAddress primaryDNS(8, 8, 8, 8);   // optional
  IPAddress secondaryDNS(8, 8, 4, 4); // optional
  
  int time = 100;

  Serial.print("Connectiog to ");

  WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(),
              IPAddress(1, 1, 1, 1), IPAddress(8, 8, 8, 8));

  WiFi.begin(_SSID, _PWD);

  Serial.println(_SSID);
  Serial.println(_PWD);

  while (WiFi.status() != WL_CONNECTED && time > 0)
  {
    Serial.print(".");
    delay(500);
    time--;
  }

  if (time != 0)
  {
    Serial.print("Connected.");
    digitalWrite(2, 1);

    SSID = String(_SSID);
    PWD = String(_PWD);

    return true;
  }
  else
  {
    Serial.print("Disconnected.");
    digitalWrite(2, 0);
    return false;
  }
}

bool connectToWiFiAgain(void)
{
  return connectToWiFi(SSID.c_str(), PWD.c_str());
}
