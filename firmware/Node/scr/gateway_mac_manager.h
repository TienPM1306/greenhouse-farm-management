#ifndef GATEWAY_MAC_MANAGER_H
#define GATEWAY_MAC_MANAGER_H

#include <Arduino.h>
#include <string.h>
#include "nvs_manager.h"

#define MAC_ADDRESS_SIZE 6
#define DEFAULT_GATEWAY_MAC {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}

#define GATEWAY_MAC_NAMESPACE "gateway_cfg"
#define GATEWAY_MAC_KEY "gateway_mac"

class GatewayMacManager
{
public:
    static GatewayMacManager *getInstance();

    /**
     * @brief Initialize GatewayMacManager with default MAC
     * @param defaultMac Pointer to 6-byte default MAC address
     */
    void initialize(const uint8_t *defaultMac = nullptr);

    /**
     * @brief Save Gateway MAC to NVS
     * @param mac Pointer to 6-byte MAC address
     * @return true if success, false otherwise
     */
    bool saveGatewayMac(const uint8_t *mac);

    /**
     * @brief Load Gateway MAC from NVS
     * @param mac Pointer to buffer where MAC will be stored (must be at least 6 bytes)
     * @return true if success, false otherwise
     */
    bool loadGatewayMac(uint8_t *mac);

    /**
     * @brief Get current Gateway MAC (from memory)
     * @param mac Pointer to buffer where MAC will be copied (must be at least 6 bytes)
     * @return true if valid MAC exists
     */
    bool getCurrentMac(uint8_t *mac);

    /**
     * @brief Check if Gateway MAC is valid (not all zeros)
     * @param mac Pointer to 6-byte MAC address
     * @return true if valid, false otherwise
     */
    bool isMacValid(const uint8_t *mac);

    /**
     * @brief Compare two MAC addresses
     * @param mac1 First MAC address
     * @param mac2 Second MAC address
     * @return true if equal, false otherwise
     */
    bool compareMac(const uint8_t *mac1, const uint8_t *mac2);

    /**
     * @brief Print MAC address to Serial
     * @param label Label to print
     * @param mac MAC address to print
     */
    void printMac(const char *label, const uint8_t *mac);

    /**
     * @brief Get stored MAC as string format (AA:BB:CC:DD:EE:FF)
     * @param mac MAC address
     * @param buffer String buffer (at least 18 bytes: "AA:BB:CC:DD:EE:FF\0")
     */
    void getMacString(const uint8_t *mac, char *buffer);

    /**
     * @brief Reset Gateway MAC to NVS (delete from storage)
     * @return true if success, false otherwise
     */
    bool resetGatewayMac();

private:
    GatewayMacManager();
    ~GatewayMacManager() = default;
    GatewayMacManager(const GatewayMacManager&) = delete;
    GatewayMacManager& operator=(const GatewayMacManager&) = delete;

    static GatewayMacManager *mInstance;
    uint8_t mCurrentMac[MAC_ADDRESS_SIZE];
    bool mIsLoaded;
};

#endif
