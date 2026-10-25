#include "device_control.h"


// STM32 Relay pin configuration
// index mang = deviceId (khop Website). index 0 bo trong (khong dung).
int relayPins[NUM_RELAYS] = {
    PC13,  // [0] khong dung (reserved)
    PB8,   // [1] Den
    PA5,   // [2] May bom
    PA4    // [3] Quat
};

bool relayStates[NUM_RELAYS] = {false};

/**
 * Initialize all relay pins as outputs and set them to LOW (off)
 */
void initRelays(void)
{
    for (int i = 0; i < NUM_RELAYS; i++)
    {
        pinMode(relayPins[i], OUTPUT);
        digitalWrite(relayPins[i], LOW);
    }
}

/**
 * Control a relay by index (0-15)
 * @param relayIndex: Relay index (0-15)
 * @param state: true = ON (HIGH), false = OFF (LOW)
 */
void controlRelay(int relayIndex, ControlDevice::ControlAction action)
{
    if (relayIndex >= 0 && relayIndex < NUM_RELAYS)
    {
        bool state = (action == ControlDevice::CONTROL_ACTION_ON);
        digitalWrite(relayPins[relayIndex], state ? HIGH : LOW);
        relayStates[relayIndex] = state;
        Serial1.print("Control Pin Index: ");
        Serial1.print(relayIndex);
        Serial1.print(" State: ");
        Serial1.println(state);
    }
}
