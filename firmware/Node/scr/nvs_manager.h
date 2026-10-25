#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include <Arduino.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_err.h>

class NVSManager
{
public:
    static NVSManager *getInstance();

    /**
     * @brief Initialize NVS
     * @return true if success, false otherwise
     */
    bool initialize();

    /**
     * @brief Save blob data to NVS
     * @param namespace_name Namespace for the data
     * @param key Key name
     * @param data Pointer to data buffer
     * @param size Size of data in bytes
     * @return true if success, false otherwise
     */
    bool saveBlob(const char *namespace_name, const char *key, const uint8_t *data, size_t size);

    /**
     * @brief Load blob data from NVS
     * @param namespace_name Namespace for the data
     * @param key Key name
     * @param data Pointer to buffer where data will be stored
     * @param size Expected size of data
     * @return true if success, false otherwise
     */
    bool loadBlob(const char *namespace_name, const char *key, uint8_t *data, size_t size);

    /**
     * @brief Check if a key exists in NVS
     * @param namespace_name Namespace for the data
     * @param key Key name
     * @return true if key exists, false otherwise
     */
    bool keyExists(const char *namespace_name, const char *key);

    /**
     * @brief Erase a key from NVS
     * @param namespace_name Namespace for the data
     * @param key Key name
     * @return true if success, false otherwise
     */
    bool eraseKey(const char *namespace_name, const char *key);

    /**
     * @brief Save uint32_t value to NVS
     * @param namespace_name Namespace for the data
     * @param key Key name
     * @param value Value to save
     * @return true if success, false otherwise
     */
    bool saveU32(const char *namespace_name, const char *key, uint32_t value);

    /**
     * @brief Load uint32_t value from NVS
     * @param namespace_name Namespace for the data
     * @param key Key name
     * @param value Pointer to store loaded value
     * @return true if success, false otherwise
     */
    bool loadU32(const char *namespace_name, const char *key, uint32_t *value);

private:
    NVSManager();
    ~NVSManager() = default;
    NVSManager(const NVSManager&) = delete;
    NVSManager& operator=(const NVSManager&) = delete;

    static NVSManager *mInstance;
    bool mInitialized;

    void printError(const char *operation, esp_err_t err);
};

#endif
