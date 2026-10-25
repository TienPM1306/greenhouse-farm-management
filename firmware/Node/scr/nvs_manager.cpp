#include "nvs_manager.h"

NVSManager *NVSManager::mInstance = nullptr;

NVSManager *NVSManager::getInstance()
{
    if (mInstance == nullptr) {
        mInstance = new NVSManager();
    }
    return mInstance;
}

NVSManager::NVSManager() : mInitialized(false)
{
}

bool NVSManager::initialize()
{
    if (mInitialized) {
        return true;
    }

    esp_err_t err = nvs_flash_init();
    
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            printError("nvs_flash_erase", err);
            return false;
        }

        // Retry nvs_flash_init
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        printError("nvs_flash_init", err);
        return false;
    }

    mInitialized = true;
    Serial.println(F("NVS Manager initialized successfully"));
    return true;
}

bool NVSManager::saveBlob(const char *namespace_name, const char *key, const uint8_t *data, size_t size)
{
    if (!mInitialized) {
        Serial.println(F("NVS Manager not initialized"));
        return false;
    }

    if (namespace_name == nullptr || key == nullptr || data == nullptr || size == 0) {
        Serial.println(F("Invalid parameters"));
        return false;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);

    if (err != ESP_OK) {
        printError("nvs_open", err);
        return false;
    }

    err = nvs_set_blob(handle, key, data, size);

    if (err != ESP_OK) {
        printError("nvs_set_blob", err);
        nvs_close(handle);
        return false;
    }

    // Commit changes
    err = nvs_commit(handle);
    nvs_close(handle);

    if (err != ESP_OK) {
        printError("nvs_commit", err);
        return false;
    }

    Serial.printf("Saved blob: %s/%s (%d bytes)\n", namespace_name, key, size);
    return true;
}

bool NVSManager::loadBlob(const char *namespace_name, const char *key, uint8_t *data, size_t size)
{
    if (!mInitialized) {
        Serial.println(F("NVS Manager not initialized"));
        return false;
    }

    if (namespace_name == nullptr || key == nullptr || data == nullptr || size == 0) {
        Serial.println(F("Invalid parameters"));
        return false;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READONLY, &handle);

    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            printError("nvs_open", err);
        }
        return false;
    }

    size_t required_size = size;
    err = nvs_get_blob(handle, key, data, &required_size);

    nvs_close(handle);

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        printError("nvs_get_blob", err);
        return false;
    }

    if (err == ESP_OK) {
        Serial.printf("Loaded blob: %s/%s (%d bytes)\n", namespace_name, key, required_size);
        return true;
    }

    return false;
}

bool NVSManager::keyExists(const char *namespace_name, const char *key)
{
    if (!mInitialized) {
        return false;
    }

    if (namespace_name == nullptr || key == nullptr) {
        return false;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READONLY, &handle);

    if (err != ESP_OK) {
        return false;
    }

    // Try to get the size of the blob
    size_t required_size = 0;
    err = nvs_get_blob(handle, key, nullptr, &required_size);
    nvs_close(handle);

    return (err == ESP_OK && required_size > 0);
}

bool NVSManager::eraseKey(const char *namespace_name, const char *key)
{
    if (!mInitialized) {
        Serial.println(F("NVS Manager not initialized"));
        return false;
    }

    if (namespace_name == nullptr || key == nullptr) {
        Serial.println(F("Invalid parameters"));
        return false;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);

    if (err != ESP_OK) {
        printError("nvs_open", err);
        return false;
    }

    err = nvs_erase_key(handle, key);

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        printError("nvs_erase_key", err);
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err != ESP_OK) {
        printError("nvs_commit", err);
        return false;
    }

    Serial.printf("Erased key: %s/%s\n", namespace_name, key);
    return true;
}

bool NVSManager::saveU32(const char *namespace_name, const char *key, uint32_t value)
{
    if (!mInitialized) {
        Serial.println(F("NVS Manager not initialized"));
        return false;
    }

    if (namespace_name == nullptr || key == nullptr) {
        Serial.println(F("Invalid parameters"));
        return false;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);

    if (err != ESP_OK) {
        printError("nvs_open", err);
        return false;
    }

    err = nvs_set_u32(handle, key, value);

    if (err != ESP_OK) {
        printError("nvs_set_u32", err);
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err != ESP_OK) {
        printError("nvs_commit", err);
        return false;
    }

    Serial.printf("Saved u32: %s/%s = %u\n", namespace_name, key, value);
    return true;
}

bool NVSManager::loadU32(const char *namespace_name, const char *key, uint32_t *value)
{
    if (!mInitialized) {
        Serial.println(F("NVS Manager not initialized"));
        return false;
    }

    if (namespace_name == nullptr || key == nullptr || value == nullptr) {
        Serial.println(F("Invalid parameters"));
        return false;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READONLY, &handle);

    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            printError("nvs_open", err);
        }
        return false;
    }

    err = nvs_get_u32(handle, key, value);
    nvs_close(handle);

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        printError("nvs_get_u32", err);
        return false;
    }

    if (err == ESP_OK) {
        Serial.printf("Loaded u32: %s/%s = %u\n", namespace_name, key, *value);
        return true;
    }

    return false;
}

void NVSManager::printError(const char *operation, esp_err_t err)
{
    Serial.print(F("NVS Error - "));
    Serial.print(operation);
    Serial.print(F(": "));
    Serial.println(err);
}
