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