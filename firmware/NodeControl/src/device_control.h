#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>
#include "message.h"

// Relay configuration (deviceId khop voi Website: 1/2/3)
// index 0 bo trong; 1 = Den (PB8), 2 = May bom (PA5), 3 = Quat (PA4)
#define NUM_RELAYS 4

// Relay index mapping (dung lam deviceId khi dieu khien)
#define RELAY_DEN     1   // Den      -> PB8
#define RELAY_MAY_BOM 2   // May bom  -> PA5
#define RELAY_QUAT    3   // Quat     -> PA4

// Relay state tracking
extern int relayPins[NUM_RELAYS];
extern bool relayStates[NUM_RELAYS];

/**
 * Initialize all relay pins as outputs and set them to LOW (off)
 */
void initRelays(void);

/**
 * Control a relay by index (0-15)
 * @param relayIndex: Relay index (0-15)
 * @param state: true = ON (HIGH), false = OFF (LOW)
 */
void controlRelay(int relayIndex, ControlDevice::ControlAction action);

#endif // DEVICE_CONTROL_H
