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

#include <unordered_map>
#include <fcntl.h>      // For open() function.
#include <unistd.h>     // For close() function.
#include <sys/ioctl.h>  // For ioctl() function.
#include <stdio.h>
#include <glib.h>
#include <vector>
 

#include "JetsonNanoGPIO.h"
#include "JetsonNanoMaps.h"
#include "ErrorHandler.h"

/**
 * Constructs a Digital GPIO pin object using based on the physical pin number.
 *
 * @param GPIO_pin_number : The GPIO physical pin number to control.
 */
GPIO_OutputPin::GPIO_OutputPin(gint GPIO_pin_number): 
GPIO_pin_number_(GPIO_pin_number) {

    g_print ("...GPIO Output - Pin %d\n", GPIO_pin_number);
    
}

/**
 * Destructor for GPIO_OutputPin. Logs the shutdown process and cleans up resources.
 */
GPIO_OutputPin::~GPIO_OutputPin() {
    g_print("Shutting down GPIO ouput pin %d (Offset %d)\n", GPIO_pin_number_, pin_offset_);
    close(req_.fd);
    g_print("Output pin closed\n");
}

/**
* Public access to ensure the pin is operational
* 
* @return : The status of the pin.
*/
gboolean GPIO_OutputPin::startedOK(){

    return started_;
}

/**
 * Setup for SysCtrl. Part of the heirachial setup chain that occurs
 * only after the camera has come online.
 *
 * @param error : Pointer the nvgstcapture-1.0 error struct for error reporting
 * 
 * @return : -1 on error, else 0.
 */

gint GPIO_OutputPin::setup(GError** error) {
    JetsonNanoPinMap tempMap;
    
    pin_offset_ = tempMap.GPIO_PinNoToOffset(GPIO_pin_number_, error);
    if (pin_offset_ == -1)
        return -1; //error should be set

    gint GPIO_output_pin_fd = open("/dev/gpiochip0", O_RDONLY);

    if (GPIO_output_pin_fd == -1) {
        g_set_error(error, g_quark_from_static_string("GPIO Error"), 1, "%s", strerror(errno));
        return -1;
    }

    req_.lineoffsets[0] = pin_offset_;
    req_.flags = GPIOHANDLE_REQUEST_OUTPUT;
    req_.lines = 1;

    gint ret = ioctl(GPIO_output_pin_fd, GPIO_GET_LINEHANDLE_IOCTL, &req_);

    close(GPIO_output_pin_fd);

    if (ret == -1) {
        g_set_error(error, g_quark_from_static_string("GPIO output pin error"), 2, "%s", strerror(errno));
        return -1;
    }

    g_print("GPIO output pin %d (Offset %d) running\n", GPIO_pin_number_, pin_offset_);

    return 0;
}

/**
 * Set the pin value, On or Off.
 *
 * @param value : 0 -> Off, !0 (probably 1) -> On
 * @param error : Pointer the nvgstcapture-1.0 error struct for error reporting.
 * 
 * @return : -1 on error, else 0.
 */
gint GPIO_OutputPin::set(guint value, GError** error) {

    data_.values[0] = value;
    gint ret = ioctl(req_.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data_);

    if (ret == -1) {
        g_set_error(error, g_quark_from_static_string("GPIO output pin error"), 3, "%s", strerror(errno));
        return -1;
    }

    return 0;
}

/**
 * Get the pin value, 0 -> Off, !0(1) -> On.
 *
 * @param error : Pointer the nvgstcapture-1.0 error struct for error reporting.
 *
 * @return : -1 on error, else pin value
 */
gint GPIO_OutputPin::get(GError** error) {

    int ret = ioctl(req_.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data_);

    if (ret == -1) {
        g_set_error(error, g_quark_from_static_string("GPIO output pin error"), 4, "%s", strerror(errno));
        return -1;
    }

    return data_.values[0];
}

/***********************************************************************************************************/


/**
 * Constructs a Digital GPIO pin object using based on the physical pin number.
 *
 * @param GPIO_pin_number : The GPIO physical pin number to control.
 */
GPIO_InputPin::GPIO_InputPin(guint GPIO_pin_number, guint debounce_time,
    guint event_flag, ErrorHandler* error_handler) :
     input_pin_channel_(nullptr), button_debounce_(FALSE), pin_number_(GPIO_pin_number),
     debounce_timeout_(debounce_time), event_flag_(event_flag), callback_handler_in_(0),
     callback_activated_(FALSE), pinFunc_(nullptr),
     error_handler_(error_handler) {
    
    g_print ("...Polling GPIO Intput - Pin %d\n", GPIO_pin_number);
}

/**
 * Destructor for GPIO_InputPin. Logs the shutdown process and cleans up resources.
 */
GPIO_InputPin::~GPIO_InputPin() {
    pinFunc_ = nullptr;
    closePin();
        g_print("Shutting down Polling GPIO Input Pin\n"); 
}

/**
 * Open and setup the digital GPIO pin.
 *
 * @param error : Pointer the nvgstcapture-1.0 error struct for error reporting
 *
 * @return : -1 on error, otherwise 0.
 */
gint GPIO_InputPin::setup(GError** error) {
    gint fd, ret;
    JetsonNanoPinMap tempMap;

    pin_offset_ = tempMap.GPIO_PinNoToOffset(pin_number_, error);
    if (pin_offset_ == -1)
        return -1; //error should be set   

    rq_.lineoffset = pin_offset_;
    rq_.eventflags = GPIOEVENT_EVENT_FALLING_EDGE;
    rq_.handleflags = GPIOHANDLE_REQUEST_INPUT;

    fd = open("/dev/gpiochip0", O_RDONLY);

    if (fd == -1) {
        g_set_error_literal(error, g_quark_from_static_string("GPIO input pin error"), 1,
            "Could not open '/dev/gpiochip0'");
        return -1;
    }

    ret = ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, &rq_);

    if (ret == -1) {
        close(fd);
        g_set_error_literal(error, g_quark_from_static_string("GPIO input pin error"), 2,
            "Failed to get line event");        
        return -1;
    }

    input_pin_channel_ = g_io_channel_unix_new(rq_.fd);
    g_io_channel_set_encoding(input_pin_channel_,NULL,NULL);

    if (!input_pin_channel_) {
        close(fd);
        g_set_error_literal(error, g_quark_from_static_string("GPIO input pin error"), 3,
            "Failed to create new GIOChannel"); 
        return -1;
    }

    callback_handler_in_ = g_io_add_watch_full(input_pin_channel_,
                    G_PRIORITY_HIGH,
                    G_IO_IN,
                    (GIOFunc)inputChangeWrapper,
                    this, NULL);  // Assuming no user_data is required for pin_callback_functions.
    g_io_channel_unref(input_pin_channel_);

    if (!callback_handler_in_) {
        g_set_error_literal(error, g_quark_from_static_string("GPIO input pin error"), 4,
            "Failed to add watch to GIOChannel");
        return -1;
        
    } else {
        callback_activated_ = TRUE;
    }

    return 0;
}

/**
* This sets the pin incomming data handling function. This can be changed as required.
*
* @param func : The data handling function to bind as the write function.
*/
void GPIO_InputPin::setPinCallbackFunction(std::function<void()> func) {
        pinFunc_ = func;        
}

/**
* This unsets the port incomming data handling function. Incomming data will be unhandled.
*
* @param func : Any dummy function, this is a work around. Ulitimately we will have a null pointer.
*/
void GPIO_InputPin::unsetPinCallbackFunction(std::function<void()> func) {
        pinFunc_ = nullptr;
}

/**
* Incommming handling function called from inputChangeInstance. Will the function
* bound to pinFunc_() at time of pin state change.
*/
void GPIO_InputPin::pinCallbackFunction() {
    if (pinFunc_ != nullptr) {    
        pinFunc_();
    }
}

/**
 * CALLBACK FUNCTION. Called when the pin state changes. This wrapper reinterprets the
 * gpointer user_data object into usable pointer for accessing the setup method in the AdditionsParent class.
 *
 * @param * src_io_channel : The io channel that received the data
 * @param cond : Triggered channel condition
 * @param data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 *
 * @return : gboolean value passed through from inputChangeInstance
 */
gboolean GPIO_InputPin::inputChangeWrapper(GIOChannel* src_io_channel, GIOCondition cond, gpointer data) {
    return reinterpret_cast<GPIO_InputPin*>(data)->inputChangeInstance(src_io_channel, cond, data);
}

/**
 * CLASS METHOD. Called when the pin state changes. We static cast the user_data pointer to access its methods.
 *
 * @param * src_io_channel : The io channel that received the data
 * @param cond : Triggered channel condition
 * @param data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 *
 * @return : return TRUE will ensure port keeps listening for Data, return FALSE on Error detection.
 */
gboolean GPIO_InputPin::inputChangeInstance(GIOChannel* src_io_channel, GIOCondition cond, gpointer data) {
    GPIO_InputPin* self = static_cast<GPIO_InputPin*>(data);
    GError* error = nullptr;
    GIOStatus status;
    gsize bytes_read = 0;
    std::vector<gchar> buffer;
    buffer.resize(32, '\0');

    if (cond & G_IO_IN) {
        status = g_io_channel_read_chars(src_io_channel, buffer.data(), buffer.size(), &bytes_read, &error);
        
        if (!self->button_debounce_){
            self->button_debounce_ = TRUE;
            g_print("\nButton pressed..\n");

            //Software debounce handling. Wait 2 seconds before accepting another button press.
            g_timeout_add(2000, self->cancelDebounceWrapper, self);
            self->pinCallbackFunction();
        }
    } else if (cond & G_IO_HUP) {
        status = g_io_channel_read_chars(src_io_channel, buffer.data(), buffer.size(), &bytes_read, &error);
        // Handle the connection broken case
        g_set_error_literal(&error, g_quark_from_static_string("GPIO input pin error"), 5,
            "Input pin connection broken");
            } else if (cond & G_IO_ERR) {
        status = g_io_channel_read_chars(src_io_channel, buffer.data(), buffer.size(), &bytes_read, &error);
        // Handle the error condition case
        g_set_error_literal(&error, g_quark_from_static_string("GPIO input pin error"), 6,
            "An error occurred on the GPIO input pin");
    }

    if (error != nullptr) {
        self->error_handler_->errorHandler(&error);
        return FALSE;  // It might be appropriate to remove this GIOChannel
    }

    // Read from the GIOChannel to clear the event    
    if (status == G_IO_STATUS_ERROR && error) {
        g_print("\nError in GIO channel\n");
        self->error_handler_->errorHandler(&error);  // Handle the error
        return FALSE;  // It might be appropriate to remove this GIOChannel
    }  
    return TRUE;
}

/**
 * CALLBACK FUNCTION. Called when a g_timeout activates. Turns off the debounce protection. 
 * This wrapper reinterprets the gpointer user_data object into usable pointer for accessing
 * the setup method in the AdditionsParent class.
 *
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 *
 * @return : gboolean value passed through from cancelDebounceInstance
 */
gboolean GPIO_InputPin::cancelDebounceWrapper(gpointer user_data) {
    return reinterpret_cast <GPIO_InputPin*>(user_data)->cancelDebounceInstance(user_data);
}

/**
 * CLASS METHOD. Called when a g_timeout activates. Turns off the debounce protection. 
 * We static cast the user_data pointer to access its methods.
 *
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 *
 * @return : return TRUE will ensure port keeps listening for Data, return FALSE on Error detection.
 */
gboolean GPIO_InputPin::cancelDebounceInstance(gpointer user_data) {
    GPIO_InputPin* self = static_cast <GPIO_InputPin*>(user_data);
    g_print("\nCancelling debounce!\n");
    self->button_debounce_ = FALSE;
    return FALSE; //Only run once
}

/**
* This method closes the GPIO port pin.
*/
void GPIO_InputPin::closePin() {
    if(rq_.fd != -1)
    {
        if(callback_activated_ == TRUE)
        {
            g_source_remove(callback_handler_in_);
            callback_activated_ = FALSE;
        }

         g_io_channel_unref(input_pin_channel_);
        close(rq_.fd);
    }
}