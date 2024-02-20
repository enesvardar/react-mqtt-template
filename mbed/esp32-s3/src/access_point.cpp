#include "access_point.h"
#include <WebServer.h>
#include <myWifi.h>
#include <string>     // std::string, std::to_string
#include <email.h>
#include "EEPROM.h"
#include "eprom.h"


String _ssid = "ESP32_" + String((uint64_t)ESP.getEfuseMac());

const char *ssid = (_ssid).c_str();        // Enter SSID here
const char *password = "123456789"; // Enter Password here

bool connected = false;

IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Set web server port number to 80
WebServer server(80);

String genHtml(void)
{

  String index_html = "<!DOCTYPE html> \n";
  index_html += "<html>\n";
  index_html += "<head>\n";
  index_html += "<title>ESP Input Form</title>\n";
  index_html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"> \n";
  index_html += "<style> \n";
  index_html += ".button {\n";
  index_html += "background-color: #4caf50;\n";
  index_html += "border-radius: 10px;\n";
  index_html += "border: none;\n";
  index_html += "color: white;\n";
  index_html += "padding: 15px 32px;\n";
  index_html += "text-align: center;\n";
  index_html += "text-decoration: none;\n";
  index_html += "display: inline-block;\n";
  index_html += "font-size: 16px;\n";
  index_html += "margin: 4px 2px;\n";
  index_html += "cursor: pointer;\n";
  index_html += "}\n";

  index_html += ".input {\n";
  index_html += "border-radius: 10px;\n";
  index_html += "width: 70%;\n";
  index_html += "height: 30%;\n";

  index_html += "padding: 12px 20px;\n";
  index_html += "margin: 8px 0;\n";
  index_html += "box-sizing: border-box;\n";
  index_html += "}\n";

  index_html += ".select {\n";
  index_html += "border-radius: 10px;\n";
  index_html += "width: 70%;\n";
  index_html += "height: 30%;\n";
  index_html += "padding: 12px 20px;\n";
  index_html += "margin: 8px 0;\n";
  index_html += "box-sizing: border-box;\n";
  index_html += "font-size: 90%;\n";
  index_html += "}\n";

  index_html += "</style>\n";
  index_html += "</head>\n";
  index_html += "<body>\n";
  index_html += "<form action=\"/submit\" method=\"post\" class=\"navbar-form pull-left\">\n";
  index_html += "<center>\n";

  index_html += "<h2>WIFI " + String((uint64_t)ESP.getEfuseMac()) + "</h2>\n";
  index_html += "<select class=\"input\" name=\"ssid\" id=\"ssid\">\n";

  int n = WiFi.scanNetworks();

  for (int i = 0; i < n; ++i) 
  {
    index_html += "<option value=\"" +  WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option> \n";
  }

  index_html += "</select>\n";

  index_html += "<p><input class=\"input\" type=\"password\" placeholder=\"Pass\" id=\"pass\" name=\"pass\"/><p/>\n";
  index_html += "<p><input class=\"input\" type=\"email\" placeholder=\"Emails\" id=\"emails\" name=\"emails\"/><p/>\n";
  index_html += "<p><button class=\"button\" type=\"submit\" >Connect</button><p/>\n";
  index_html += "<center/>\n";
  index_html += "</form>\n";
  index_html += "</body>\n";
  index_html += "</html>\n";

  return index_html;
}

void handleOnConnect()
{
  server.send(200, "text/html", genHtml().c_str());
}

void handlePost()
{
  connected = false;

  String user_ssid = server.arg(0);
  String user_pass = server.arg(1);
  String email = server.arg(2);

  Serial.println(user_ssid);
  Serial.println(user_pass);

  if (user_ssid.length() > 0 && user_pass.length() > 0)
  {
    connected = connectToWiFi(user_ssid.c_str(), user_pass.c_str());
  }
  
  if (connected == false)
  {
    server.send(200, "text/html", genHtml().c_str());
  }
  else
  {
    setEmail(email);
    server.close();
    WiFi.softAPdisconnect(true);

    EEPROM.write(0, user_ssid.length());
    EEPROM.write(1, user_pass.length());
    EEPROM.commit();

    writeEpromString(user_ssid.c_str(),2,user_ssid.length());
    writeEpromString(user_pass.c_str(),3 + user_ssid.length(),user_pass.length());
  }
}

void accesPointInit()
{
  connected = false;
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);

  server.on("/", handleOnConnect);
  server.on("/submit", HTTP_POST, handlePost);
  server.begin();
}

void accesPointLoop()
{
  while (!connected)
  {
    server.handleClient();
  }
}
