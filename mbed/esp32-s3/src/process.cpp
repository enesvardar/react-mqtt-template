#include <list>
#include "time.h"
#include <Arduino.h>
#include <myMqtt.h>
#include <iostream>
#include <vector>
#include "config.h"
#include <string>
#include "esp_ota_ops.h"
#include <HTTPClient.h>

bool PROCESS_FLAG = false;

using namespace std;

esp_err_t updateFirmware()
{
    HTTPClient http;

    string url = "https://raw.githubusercontent.com/enesvardar/firmware/main/firmware.bin";

    http.begin(url.c_str()); // İndirilecek dosyanın URL'si

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {

        String firmwareData = http.getString(&mqttClient);

        const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);

        if (partition == NULL)
        {
            Serial.println("Failed to get OTA update partition");
            return ESP_FAIL;
        }

        Serial.printf("Writing firmware to partition '%s' at offset 0x%x\n", partition->label, partition->address);

        esp_ota_handle_t otaHandle;
        esp_err_t err = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &otaHandle);
        if (err != ESP_OK)
        {
            Serial.printf("Failed to begin OTA update, error code: %d\n", err);
            return err;
        }

        size_t writtenBytes = 0;

        char buffer[1024];
        int readBytes = 0;
        int i = 0;

        while (i < firmwareData.length())
        {
            mqttClient.loop();

            if (!mqttClient.connected())
            {
                reconnectTry();
            }

            readBytes = ((firmwareData.length() - i) > sizeof(buffer)) ? sizeof(buffer) : (firmwareData.length() - i);
            memcpy(buffer, &firmwareData[i], readBytes);
            err = esp_ota_write(otaHandle, buffer, readBytes);
            if (err != ESP_OK)
            {
                Serial.printf("Failed to write OTA data, error code: %d\n", err);
                return err;
            }

            i += readBytes;
            writtenBytes += readBytes;
            // Serial.printf("Written %d bytes to OTA partition...\n", writtenBytes);

            int state = (int)((float)(i * 100) / (float)firmwareData.length());

            Serial.printf("%d%\n", state);
            MqttResponse.sendUpdateInfo(String(state));
        }

        err = esp_ota_end(otaHandle);

        if (err != ESP_OK)
        {
            Serial.printf("Failed to end OTA update, error code: %d\n", err);
            return err;
        }

        err = esp_ota_set_boot_partition(partition);

        if (err != ESP_OK)
        {
            Serial.printf("Failed to set boot partition, error code: %d\n", err);
            return err;
        }

        Serial.println("Firmware update complete. Rebooting...");
        esp_restart();

        return ESP_OK;
    }

    http.end();
}

void processLoop(void)
{
    if (MqttRequest.cmd != "")
    {
        Serial.println(MqttRequest.cmd);

        PROCESS_FLAG = true;

        if (MqttRequest.cmd == "CMD_UPDATE_FIRMWARE")
        {
            updateFirmware();
        }
        else if (MqttRequest.cmd == "CMD_PING")
        {
            MqttResponse.sendPing(PROCESS_FLAG);
        }

        PROCESS_FLAG = false;

        MqttRequest.clear();
    }

    delay(1);
}