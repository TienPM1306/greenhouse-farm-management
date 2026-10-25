#include <Arduino.h>
#include "device_control.h"
#include "esp_service.h"

/**
 * Initialize STM32 relay control system
 */
void setup()
{
    // Debug log goes to CP2101 on USART1 (PA9 = TX, PA10 = RX).
    Serial1.begin(9600);
    Serial1.println("STM32 Relay Control Initialized");
    
    Serial_Setup(9600);
    initRelays();
}

/**
 * Main application loop - handle serial commands and relay control
 */
void loop()
{
    // Check for incoming serial data
    if (Serial3.available())
    {
        // Read a full frame. The STM32 drains the UART far faster than bytes
        // arrive at 9600 baud (~1 ms/byte), so a plain while(available()) loop
        // would return after the very first byte. Wait for a short inter-byte
        // gap (20 ms of silence) to decide the frame is complete.
        uint8_t buffer[256];
        uint16_t length = 0;
        unsigned long lastByteMs = millis();

        while (millis() - lastByteMs < 20)
        {
            if (Serial3.available() && length < 256)
            {
                buffer[length++] = Serial3.read();
                lastByteMs = millis();
            }
        }

        // Process and print received data
        if (length > 0)
        {
            processerData(buffer, length);
        }
    }

    delay(50);
}
