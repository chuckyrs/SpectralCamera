#include "JetsonNanoMaps.h"

JetsonNanoPinMap::JetsonNanoPinMap() {
    pin_map_ = {
        {7, 216},
        {11, 165},
        {12, 166},
        {13, 396},
        {15, 397},
        {16, 255},
        {18, 429},
        {29, 428},
        {31, 427},
        {32, 389},
        {33, 395},
        {35, 388},
        {36, 392},
        {37, 296},
        {38, 193},
        {40, 194}
    };
}

gint JetsonNanoPinMap::GPIO_PinNoToOffset(guint pin_number, GError** error) {
    if(pin_map_.find(pin_number) == pin_map_.end()){
        g_set_error(
            error,                // GError **
            g_quark_from_static_string("pin mapping"), // Error domain 
            1,                    // Error code
            "Pin number %d not valid for general IO use", // Error message
            pin_number);
        return -1;
    }
    return pin_map_[pin_number];
}

JetsonNanoDeviceMap::JetsonNanoDeviceMap() {
    device_map_ = {
        {"USB0", "/dev/ttyUSB0"},
        {"USB1", "/dev/ttyUSB1"},
        {"USB2", "/dev/ttyUSB2"},
        {"UART0", "/dev/ttyS0"},
        {"UART1", "/dev/ttyTHS1"},
        {"UART2", "/dev/ttyTHS2"},
        {"camera-0", "/dev/i2c-8"},
        {"camera-1", "/dev/i2c-7"}
    };
}

std::string JetsonNanoDeviceMap::identifierToDevice(const std::string &identifier, GError** error) {
    // Look up the port string using the provided identifier
    auto it = device_map_.find(identifier);
    if (it == device_map_.end()) {
        g_set_error(
            error,                // GError **
            g_quark_from_static_string("device mapping"), // Error domain 
            1,                    // Error code
            "Device ID '%s' not available for use",  // Error message
            identifier.c_str());
        return NULL;
    }

    // Use the found port string
    return it->second;
}