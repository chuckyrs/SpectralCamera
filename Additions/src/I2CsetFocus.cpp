#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/i2c-dev.h>

extern "C" { //You F....n BEAUTY!!
    #include <i2c/smbus.h>
}

#include "I2CsetFocus.h"
#include "JetsonNanoMaps.h"
#include "ErrorHandler.h"

CameraI2CDevice::CameraI2CDevice(const std::string& camera_id ) : camera_id_(camera_id),
    camera_i2c_fd_(-1) {
    g_print ("...i2c focus controller for %s\n", camera_id_.c_str());        
}

CameraI2CDevice::~CameraI2CDevice() {
    g_print("Shutting down I2C focus controller for %s\n", camera_id_.c_str());
    if (camera_i2c_fd_ >= 0) {
        close(camera_i2c_fd_);

    g_print("I2C focus controller closed");
    }
}

gint CameraI2CDevice::setup(GError** error){

    JetsonNanoDeviceMap camera_to_device;
    std::string camera_device = camera_to_device.identifierToDevice(camera_id_, error);

    if (camera_device.empty())
        return -1; //error should be set
    
    camera_i2c_fd_ = open(camera_device.c_str(), O_RDWR);
    if (camera_i2c_fd_ < 0) {
        g_set_error(error, g_quark_from_static_string("i2c device"), 1,
        "Failed to open i2c device '%s'", camera_device.c_str());            
        return -1;
    }

    g_print("I2C focus controller %s open on device %s\n", camera_id_.c_str(), camera_device.c_str());

    return 0;
}

gint CameraI2CDevice::setFocus(int range, GError** error) {
    __u8 byte1, byte2;
    __s32 res;
    unsigned int value;

    value = (range<<4) & 0x3ff0;
    byte1 = (value>>8) & 0x3f;
    byte2 = value & 0xf0;    

    if (ioctl(camera_i2c_fd_, I2C_SLAVE, 0x0C) < 0) {
        g_set_error_literal(error, g_quark_from_static_string("i2c device"), 1,
         "ioctl(I2CSLAVE) failed in setFocus");
        return -1;
    }

    res = i2c_smbus_write_byte_data(camera_i2c_fd_, byte1, byte2);
    if (res<0) {
        g_set_error_literal(error, g_quark_from_static_string("i2c device"), 1,
        "I2C write failed in setFocus");
        return -1;
    }

    return 0;
}

/*static I2CDevice::I2CDevice* create(const std::string& camera) {
    return new I2CDevice(camera);
}*/

/*Need unified error handling

GError* CameraI2CDevice::getError() {
    return error_;
}

void CameraI2CDevice::handleError(const std::string& errMsg) {
    g_set_error(&error_, g_quark_from_static_string("I2CDevice"), 1, "%s", errMsg.c_str());
    close(camera_i2c_fd_);
}*/