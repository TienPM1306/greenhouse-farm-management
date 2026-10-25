#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>

#define RESULT_OK       0u
#define RESULT_ERR      1u
#define RESULT_NO_DATA  2u

static bool getMacAddress(uint8_t *mac)
{
    if (mac == nullptr) {
        return false;
    }
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
        Serial.printf("Failed to read MAC address: %d\n", err);
        return false;
    }
    return true;
}

static void formatMacAddress(const uint8_t *mac, char *buffer, size_t len)
{
    snprintf(buffer, len, "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

#endif
