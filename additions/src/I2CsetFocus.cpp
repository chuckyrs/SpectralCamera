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

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/i2c-dev.h>

extern "C" { //smbus.h needs to be handled by a C compiler
    #include <i2c/smbus.h>
}

#include "I2CsetFocus.h"
#include "JetsonNanoMaps.h"
#include "ErrorHandler.h"

/**
 * Constructs a CameraI2CDevice object associated with a specific camera using its identifier.
 *
 * @param camera_id : A string identifier for the camera to be controlled through I2C.
 */
CameraI2CDevice::CameraI2CDevice(const std::string& camera_id ) : camera_id_(camera_id),
    camera_i2c_fd_(-1) { // Initialize file descriptor to invalid value
    g_print ("...i2c focus controller for %s\n", camera_id_.c_str());  // Log the initialization of I2C controller for the specified camera      
}

/**
 * Destructor for CameraI2CDevice. Cleans up by closing the I2C device if it's open and logs the shutdown.
 */
CameraI2CDevice::~CameraI2CDevice() {
    g_print("Shutting down I2C focus controller for %s\n", camera_id_.c_str());
    if (camera_i2c_fd_ >= 0) { 
        close(camera_i2c_fd_); // Close the file descriptor if it's valid

    g_print("I2C focus controller closed"); // Confirm the I2C controller has been closed
    }
}

/**
 * Initializes the I2C device for the specified camera.
 *
 * @param error : Double pointer to a GError structure to allow error information to be passed in and modified.
 * @return gint : Returns 0 on success or -1 on failure, setting the error if an issue occurs.
 */
gint CameraI2CDevice::setup(GError** error){

    JetsonNanoDeviceMap camera_to_device;
    std::string camera_device = camera_to_device.identifierToDevice(camera_id_, error); // Maps camera_id to a device path

    if (camera_device.empty())
        return -1; // Return error if device mapping fails
    
    camera_i2c_fd_ = open(camera_device.c_str(), O_RDWR); // Attempt to open the I2C device
    if (camera_i2c_fd_ < 0) {
        g_set_error(error, g_quark_from_static_string("i2c device"), 1,
        "Failed to open i2c device '%s'", camera_device.c_str());            
        return -1; // Return error if device cannot be opened
    }

    g_print("I2C focus controller %s open on device %s\n", camera_id_.c_str(), camera_device.c_str());

    return 0;
}

/**
 * Sets the focus to a specific range on the camera's I2C device.
 *
 * @param range : Integer specifying the focus range value to set.
 * @param error : Double pointer to a GError structure to allow error information to be passed in and modified.
 * @return gint : Returns 0 on successful operation, -1 on failure.
 */
gint CameraI2CDevice::setFocus(int range, GError** error) {
    __u8 byte1, byte2; // Define variables for the two parts of the I2C message
    __s32 res;
    unsigned int value;

    value = (range<<4) & 0x3ff0; // Format the range into the expected format for I2C communication
    byte1 = (value>>8) & 0x3f;  // Extract high byte
    byte2 = value & 0xf0;    // Extract low byte

    if (ioctl(camera_i2c_fd_, I2C_SLAVE, 0x0C) < 0) {
        g_set_error_literal(error, g_quark_from_static_string("i2c device"), 1,
         "ioctl(I2CSLAVE) failed in setFocus");
        return -1; // Return error if setting the device address fails
    }

    res = i2c_smbus_write_byte_data(camera_i2c_fd_, byte1, byte2);
    if (res<0) {
        g_set_error_literal(error, g_quark_from_static_string("i2c device"), 1,
        "I2C write failed in setFocus");
        return -1; // Return error if the write operation fails
    }

    return 0;
}