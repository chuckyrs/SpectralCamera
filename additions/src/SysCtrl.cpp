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


/**
 * Constructs a SysCtrl object which responds to the button press and coordinates
 * the hardware response.
 *
 * @param main_context : A string identifier for the camera to be controlled through I2C.
 * @param * additions_parent : Pointer to the application's addition_parent object
 * @param * output_file_control : Pointer to the application's output_file_control object
 * @param * error_handler : Pointer to the application's error_handler object
 * 
 */
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
    as7265x_unit_(&usb0_serial_port_, output_file_control_, error_handler_) {  
        g_print ("...System Controller\n");    
}

/**
 * Destructor for SysCtrl. Logs the shutdown process and cleans up resources.
 */
SysCtrl::~SysCtrl(){
    g_print("System controller removing components...\n");
}

/**
 * Setup for SysCtrl. Part of the heirachial setup chain that occurs
 * only after the camera has come online.
 * 
 * @param error : Pointer to the nvgstcapture-1.0 error struct for error reporting
 * 
 * @return : -1 on error, else 0.
 */
gint SysCtrl::setup(GError** error) {

    //Setup the serial port first as it is the most likely
    //to be unplugged.

    gboolean errorDuringSetup = FALSE;

    if (!errorDuringSetup)
        errorDuringSetup = ((usb0_serial_port_.setup(error)) == -1);
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

/**
 * Get the handshake data for the connected as7265x board. The as7265x
 * object is already created.
 */
void SysCtrl::run_ams7265xHandshake() {
    as7265x_unit_.getHandshakeData();
}

/**
 * CALLBACK FUNCTION. Turn off the LEDs via pins 38 and 40 after a timeout. This wrapper reinterprets the 
 * gpointer user_data object into usable pointer for accessing the setup method in the AdditionsParent class.
 *
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 */
gboolean SysCtrl::GPIO_LightsOutWrapper(gpointer user_data) {
        return reinterpret_cast<SysCtrl*>(user_data)->GPIO_LightsOut(user_data);
}

/**
 * CLASS METHOD. Turn off the LEDs via pins 38 and 40. We static cast the user_data pointer to access its methods.
 *
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 * 
 * @return : return FALSE will ensure the timeout does not run again.
 */
gboolean SysCtrl::GPIO_LightsOut(gpointer user_data) {
    SysCtrl* self = static_cast<SysCtrl*>(user_data);

     GError *error = NULL;

    if ((self->output_pin_40_.set(1, &error) == -1 ) ||
            (self->output_pin_38_.set(1, &error) == -1 ))
        error_handler_->errorHandler(&error);

    return FALSE;
}

/**
 * CALLBACK FUNCTION. Turn on the ambient LED via pin 40 after g_timeout. This wrapper reinterprets the 
 * gpointer user_data object into usable pointer for accessing the setup method in the AdditionsParent class.
 *
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 */
gboolean SysCtrl::GPIO_AmbientOnWrapper(gpointer user_data) {
        return reinterpret_cast<SysCtrl*>(user_data)->GPIO_AmbientOn(user_data);
}

/**
 * CLASS METHOD. Turn on the ambient LED via pin 40 after g_timeout. We static cast the user_data pointer to access its methods.
 *
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 *
 * @return : return FALSE will ensure the timeout does not run again.
 */
gboolean SysCtrl::GPIO_AmbientOn(gpointer user_data) {
    SysCtrl* self = static_cast<SysCtrl*>(user_data);

    GError *error = NULL;

    if (self->output_pin_40_.set(1, &error) == -1)
        error_handler_->errorHandler(&error);

    return FALSE;
}

/**
 * CALLBACK FUNCTION. Turn on the flash LED via pin 38 after g_timeout. This wrapper reinterprets the 
 * gpointer user_data object into usable pointer for accessing the setup method in the AdditionsParent class.
 *
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 */
gboolean SysCtrl::GPIO_FlashOnWrapper(gpointer user_data) {
        return reinterpret_cast<SysCtrl*>(user_data)->GPIO_FlashOn(user_data);
}

/**
 * CLASS METHOD. Turn on the flash LED via pin 38 after g_timeout. We static cast the user_data pointer to access its methods.
 *
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 *
 * @return : return FALSE will ensure the timeout does not run again.
 */
gboolean SysCtrl::GPIO_FlashOn(gpointer user_data) {
    SysCtrl* self = static_cast<SysCtrl*>(user_data);

    GError *error = NULL;

    if (self->output_pin_38_.set(1, &error) == -1)
        error_handler_->errorHandler(&error);

    return FALSE;
}
 
/**
* This method is not a callback function. Instead it is bound a response function so it can be substituted if
* different responses are required to a button press. This means that this function does not need re-casting.
* 
* This is method is central to determining the timing of the response to the button press. Changing the timeouts
* here will allow synchronising the image and spectral data collection.
*/
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