#ifndef JETSONNANOMAPS_H
#define JETSONNANOMAPS_H

#include <unordered_map>
#include <string>
#include <glib.h>

class JetsonNanoPinMap {
public:
    JetsonNanoPinMap();
    gint GPIO_PinNoToOffset(guint pin_number, GError** error);

private:
    std::unordered_map<gint, gint> pin_map_;   
};

class JetsonNanoDeviceMap {
public:
    JetsonNanoDeviceMap();
    std::string identifierToDevice(const std::string &indentifier, GError** error);

private:
    std::unordered_map<std::string, std::string> device_map_;   
};

#endif //JETSONNANOMAPS_H