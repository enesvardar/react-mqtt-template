#include <Arduino.h>
#include <PubSubClient.h>
#include <vector>

using namespace std;

extern PubSubClient mqttClient;

class mqttRequest
{

public:
    String cmd;
    int priority;
    vector<String> dataList;

    void clear(void)
    {
        this->cmd = "";
        this->priority = 0;
        this->dataList.clear();
    }

    void payloadParser(byte *payload, int length)
    {
        this->dataList.clear();

        String msg = "";

        for (int i = 0; i < length; i++)
        {
            if ((char)payload[i] != '/')
            {
                msg += ((char)payload[i]);
            }
            else
            {
                this->dataList.push_back(msg);
                msg = "";
            }
        }

        this->dataList.push_back(msg);

        this->cmd = this->dataList[0];

        Serial.println(this->cmd);
    }

    mqttRequest()
    {
        this->clear();
    }
};

extern mqttRequest MqttRequest;

class mqttResponse
{

public:
    String mac;
    String topicNameNODE;

    void sendMqttData(const char *data)
    {
        mqttClient.publish(this->topicNameNODE.c_str(), data);
    }

    void sendPing(bool processFlag)
    {
        if (processFlag == true)
        {
            this->sendMqttData((mac + String("/CMD_PING/BUSY")).c_str());
        }
        else
        {
            this->sendMqttData((mac + String("/CMD_PING/NOT_BUSY")).c_str());
        }
    }

    void sendUpdateInfo(String state)
    {
        this->sendMqttData((mac + String("/CMD_UPDATE_FIRMWARE/") + state).c_str());
    }

    mqttResponse()
    {
        this->mac = String((uint64_t)ESP.getEfuseMac());
        this->topicNameNODE = "/gtsField1/NODEJS";
    }
};

extern mqttResponse MqttResponse;

void callback(char *topic, byte *payload, unsigned int length);
void setupMQTT();
void reconnect();
void reconnectTry();