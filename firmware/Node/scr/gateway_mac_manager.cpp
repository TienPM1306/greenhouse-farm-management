#include "gateway_mac_manager.h"

GatewayMacManager *GatewayMacManager::mInstance = nullptr;

GatewayMacManager *GatewayMacManager::getInstance()
{
    if (mInstance == nullptr) {
        mInstance = new GatewayMacManager();
    }
    return mInstance;
}

GatewayMacManager::GatewayMacManager() : mIsLoaded(false)
{
    memset(mCurrentMac, 0, MAC_ADDRESS_SIZE);
}

void GatewayMacManager::initialize(const uint8_t *defaultMac)
{
    // Initialize NVS
    NVSManager *nvs = NVSManager::getInstance();
    if (!nvs->initialize()) {
        Serial.println(F("Failed to initialize NVS"));
    }

    // Try to load MAC from NVS
    if (loadGatewayMac(mCurrentMac)) {
        mIsLoaded = true;
        Serial.print(F("Loaded Gateway MAC from NVS: "));
        printMac("", mCurrentMac);
    } else {
        // Use default MAC if provided
        if (defaultMac != nullptr) {
            memcpy(mCurrentMac, defaultMac, MAC_ADDRESS_SIZE);
            mIsLoaded = true;
            Serial.print(F("Using default Gateway MAC: "));
            printMac("", mCurrentMac);
        } else {
            // Set default MAC
            uint8_t default_mac[] = DEFAULT_GATEWAY_MAC;
            memcpy(mCurrentMac, default_mac, MAC_ADDRESS_SIZE);
            mIsLoaded = true;
            Serial.print(F("Using built-in default Gateway MAC: "));
            printMac("", mCurrentMac);
        }
    }
}

bool GatewayMacManager::saveGatewayMac(const uint8_t *mac)
{
    if (mac == nullptr) {
        Serial.println(F("Invalid MAC address pointer"));
        return false;
    }

    if (!isMacValid(mac)) {
        Serial.println(F("Invalid MAC address (all zeros)"));
        return false;
    }

    NVSManager *nvs = NVSManager::getInstance();
    if (!nvs->saveBlob(GATEWAY_MAC_NAMESPACE, GATEWAY_MAC_KEY, mac, MAC_ADDRESS_SIZE)) {
        Serial.println(F("Failed to save Gateway MAC to NVS"));
        return false;
    }

    // Update current MAC
    memcpy(mCurrentMac, mac, MAC_ADDRESS_SIZE);
    mIsLoaded = true;
    return true;
}

bool GatewayMacManager::loadGatewayMac(uint8_t *mac)
{
    if (mac == nullptr) {
        Serial.println(F("Invalid MAC address buffer"));
        return false;
    }

    NVSManager *nvs = NVSManager::getInstance();
    if (!nvs->loadBlob(GATEWAY_MAC_NAMESPACE, GATEWAY_MAC_KEY, mac, MAC_ADDRESS_SIZE)) {
        return false;
    }

    return true;
}

bool GatewayMacManager::getCurrentMac(uint8_t *mac)
{
    if (mac == nullptr) {
        Serial.println(F("Invalid MAC address buffer"));
        return false;
    }

    if (!mIsLoaded) {
        Serial.println(F("Gateway MAC not loaded"));
        return false;
    }

    memcpy(mac, mCurrentMac, MAC_ADDRESS_SIZE);
    return true;
}

bool GatewayMacManager::isMacValid(const uint8_t *mac)
{
    if (mac == nullptr) {
        return false;
    }

    // Check if all bytes are zero
    for (int i = 0; i < MAC_ADDRESS_SIZE; i++) {
        if (mac[i] != 0) {
            return true;
        }
    }

    return false;
}

bool GatewayMacManager::compareMac(const uint8_t *mac1, const uint8_t *mac2)
{
    if (mac1 == nullptr || mac2 == nullptr) {
        return false;
    }

    return (memcmp(mac1, mac2, MAC_ADDRESS_SIZE) == 0);
}

void GatewayMacManager::printMac(const char *label, const uint8_t *mac)
{
    if (mac == nullptr) {
        return;
    }

    if (label && strlen(label) > 0) {
        Serial.print(label);
        Serial.print(F(": "));
    }

    for (int i = 0; i < MAC_ADDRESS_SIZE; i++) {
        Serial.printf("%02X", mac[i]);
        if (i < MAC_ADDRESS_SIZE - 1) {
            Serial.print(F(":"));
        }
    }
    Serial.println();
}

void GatewayMacManager::getMacString(const uint8_t *mac, char *buffer)
{
    if (mac == nullptr || buffer == nullptr) {
        return;
    }

    sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool GatewayMacManager::resetGatewayMac()
{
    NVSManager *nvs = NVSManager::getInstance();
    if (!nvs->eraseKey(GATEWAY_MAC_NAMESPACE, GATEWAY_MAC_KEY)) {
        Serial.println(F("Failed to reset Gateway MAC"));
        return false;
    }

    // Reset current MAC to default
    uint8_t default_mac[] = DEFAULT_GATEWAY_MAC;
    memcpy(mCurrentMac, default_mac, MAC_ADDRESS_SIZE);
    return true;
}
