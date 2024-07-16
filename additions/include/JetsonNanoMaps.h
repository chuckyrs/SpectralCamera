/*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files(the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and /or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions :
*
*The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
*THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

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