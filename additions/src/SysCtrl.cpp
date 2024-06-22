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

#include <sys/stat.h>
#include <sstream>
#include <iomanip>
#include <memory>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>
#include <dirent.h>
#include <iostream>
#include <cstring>


#include "SysCtrl.h"
#include "AdditionsParent.h"
#include "ErrorHandler.h"
/*#ifdef __cplusplus
extern "C" {
#endif
#include "nvgstcapture.h"
#ifdef __cplusplus
}
#endif*/

//Maybe better called SysCtrl becuase it does more than just light control.
//Actually, this is the centre of all additions.
SysCtrl::SysCtrl(GMainContext* main_context, 
    AdditionsParent* additions_parent,
    OutputFileControl* output_file_control,
    ErrorHandler* error_handler) 
    : main_context_(main_context),focusLock(FALSE),
    additions_parent_(additions_parent),
    output_file_control_(output_file_control),
    error_handler_(error_handler),  //Pin 7 is offset 216
    input_pin_7_(7, 2000, GPIOEVENT_EVENT_FALLING_EDGE, error_handler_),
    output_pin_38_(38), //Pin 38, Offset 77  -  FLASH
    output_pin_40_(40), //Pin 40, Offset 78 - AMBIENT
    usb0_serial_port_("USB0",115200,8,1,0,0, error_handler_),
    as7265x_unit_(&usb0_serial_port_, output_file_control_, error_handler_)   
    {  //GError* error=nullptr;
        //gint ret = input_pin_7_.setup(&error);
        g_print ("...System Controller\n"); 
    
}

SysCtrl::~SysCtrl(){
    g_print("System controller removing components...\n");
}

gint SysCtrl::setup(GError** error) {

    //Setup the serial port first as it is the most likely
    //to be unplugged.

    gboolean errorDuringSetup = FALSE;

    if (!errorDuringSetup)
        errorDuringSetup = ((usb0_serial_port_.setup(error)) == -1 );
    if (!errorDuringSetup)
        errorDuringSetup = ((input_pin_7_.setup(error)) == -1);
    if (!errorDuringSetup)
        errorDuringSetup = ((output_pin_38_.setup(error))  == -1);
    if (!errorDuringSetup)
        errorDuringSetup = ((output_pin_40_.setup(error)) == -1);

    if (!errorDuringSetup){
        input_pin_7_.setPinCallbackFunction(std::bind(&SysCtrl::GPIO_InputPinChange,
            this));
        g_print ("System controller setup\n");
        return 0;
    }
    else
        return -1;
}

void SysCtrl::run_ams7265xHandshake() {
    as7265x_unit_.getHandshakeData();
}

gboolean SysCtrl::GPIO_LightsOutWrapper(gpointer user_data) {
        return reinterpret_cast<SysCtrl*>(user_data)->GPIO_LightsOut(user_data);
}

gboolean SysCtrl::GPIO_LightsOut(gpointer user_data) {
    SysCtrl* self = static_cast<SysCtrl*>(user_data);

     GError *error = NULL;

    if ((self->output_pin_40_.set(1, &error) == -1 ) ||
            (self->output_pin_38_.set(1, &error) == -1 ))
        error_handler_->errorHandler(&error);

    return FALSE;
}

gboolean SysCtrl::GPIO_AmbientOnWrapper(gpointer user_data) {
        return reinterpret_cast<SysCtrl*>(user_data)->GPIO_AmbientOn(user_data);
}

gboolean SysCtrl::GPIO_AmbientOn(gpointer user_data) {
    SysCtrl* self = static_cast<SysCtrl*>(user_data);

    GError *error = NULL;

    if (self->output_pin_40_.set(1, &error) == -1)
        error_handler_->errorHandler(&error);

    return FALSE;
}

gboolean SysCtrl::GPIO_FlashOnWrapper(gpointer user_data) {
        return reinterpret_cast<SysCtrl*>(user_data)->GPIO_FlashOn(user_data);
}

gboolean SysCtrl::GPIO_FlashOn(gpointer user_data) {
    SysCtrl* self = static_cast<SysCtrl*>(user_data);

    GError *error = NULL;

    if (self->output_pin_38_.set(1, &error) == -1)
        error_handler_->errorHandler(&error);

    return FALSE;
}
 // static wrappers
/*gboolean SysCtrl::GPIO_InputPinChangeWrapper(gpointer user_data) {
        return reinterpret_cast<SysCtrl*>(user_data)->GPIO_InputPinChange(user_data);
}*/

void SysCtrl::GPIO_InputPinChange() {
    
    additions_parent_->af_iface_.setFocusLock();
    output_file_control_->setButtonTriggered();
    output_file_control_->captureDataTime();

    g_timeout_add_full(G_PRIORITY_DEFAULT, 3800, GPIO_LightsOutWrapper, this, nullptr);
    g_timeout_add_full(G_PRIORITY_DEFAULT, 4000, GPIO_AmbientOnWrapper, this, nullptr);
    g_timeout_add_full(G_PRIORITY_DEFAULT, 4000, additions_parent_->af_iface_.releaseFocusLockWrapper,
        &additions_parent_->af_iface_, nullptr);
    g_timeout_add_full(G_PRIORITY_DEFAULT, 200, GPIO_FlashOnWrapper, this, nullptr);
    g_timeout_add_full(G_PRIORITY_DEFAULT, 3600, as7265x_unit_.getAS7265xDataWrapper,
    &as7265x_unit_, nullptr);
    g_timeout_add_full(G_PRIORITY_DEFAULT, 100, GPIO_LightsOutWrapper, this, nullptr);    

}