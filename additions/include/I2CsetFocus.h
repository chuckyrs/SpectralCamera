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

#ifndef I2CSETFOCUS_H
#define I2CSETFOCUS_H

#include <glib.h>
#include <string>

typedef unsigned char u8;

class ErrorHandler;

class CameraI2CDevice {
public:
    CameraI2CDevice(const std::string& camera_id);
    ~CameraI2CDevice();
    gint setup(GError** error);
    gint setFocus(gint range, GError** error);

private:
    int camera_i2c_fd_;
    std::string camera_id_;
};
#endif //I2CSETFOCUS_H