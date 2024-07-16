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

#include "AdditionsParent.h"

/**
 * Constructs an AdditionsParent object.
 *
 * @param main_context : A string identifier for the camera to be controlled through I2C.
 * @param * width : A pointer the curent resolution width in nvgstcapture-1.0
 * @param * height : A pointer the curent resolution height in nvgstcapture-1.0
 * @param trigger_image_capture : A function pointer to an nvgstcapture-1.0 function to
                                  trigger image capture.
 * @param additions_exit_capture :  A function pointer to an nvgstcapture-1.0 function to
                                  trigger application exit due to error.
 * @param focus_valve_open :  A function pointer to an nvgstcapture-1.0 function to
                                  allow focus frames to pass through.
 * @param focus_valve_close :  A function pointer to an nvgstcapture-1.0 function to
                                  stop focus frames passing through.
 * @param error : Pointer the nvgstcapture-1.0 error struct for error reporting
 */
AdditionsParent::AdditionsParent(GMainContext* main_context, gint* width, gint* height,
    TriggerImageCapture trigger_image_capture, AdditionsExitCapture additions_exit_capture,
    FocusValveOpen focus_valve_open, FocusValveClose focus_valve_close, GError** error) 
    : main_context_(main_context), width_(width), height_(height), trigger_image_capture_(trigger_image_capture),
    additions_exit_capture_(additions_exit_capture), focus_valve_open_(focus_valve_open),
    focus_valve_close_(focus_valve_close), error_(error),
    error_handler_(this),
    output_file_control_("/home/New_Data/", &error_handler_), //Need to remove the string from here
    system_control_(main_context, this, &output_file_control_, &error_handler_),
    af_iface_(this, &error_handler_) {

}

/**
 * Destructor for AdditionsParent. Logs the shutdown process and cleans up resources.
 */
AdditionsParent::~AdditionsParent(){
    g_print("Closing and removing all additional objects...\n");

}

/**
 * This is where the hierachial setup of all objects begins. Nvgstcapture-1.0 runs this after the camera object
 * comes online. This wrapper reinterprets the gpointer user_data object into usable pointer for accessing
 * the setup method in the AdditionsParent class.
 * 
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 */
void AdditionsParent::runSetupWrapper(gpointer user_data) {
        reinterpret_cast<AdditionsParent*>(user_data)->setup(user_data);
}

/**
 *This method is called after the camera is operational to setup all the C++ objects. If the AS7265x
 * is not connected via UART the application will error and exit. Each dependant layer cascades through its
 * dependant objects.
 * 
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 */
void AdditionsParent::setup(gpointer user_data) {
    
    //Check whether this line is required, doesn't seem to be.
    AdditionsParent* self = static_cast<AdditionsParent*>(user_data);

    GError* error = nullptr;
    gboolean errorDuringSetup = FALSE;
    g_print("\nSetting up additional objects...\n");
    
    //Setup the serial port first as it is the most likely
    //to be unplugged.

    if (!errorDuringSetup)
        errorDuringSetup = ((system_control_.setup(&error)) == -1);
    if (!errorDuringSetup)
        errorDuringSetup = ((af_iface_.setup(&error)) == -1);
    if (!errorDuringSetup)
        errorDuringSetup = ((output_file_control_.setup(&error)) == -1);
    if (!errorDuringSetup)
        system_control_.run_ams7265xHandshake();
    else
        errorShutdown(&error);  
}

/**
 * This wrapper reinterprets the gpointer user_data object into usable pointer for accessing
 * the focusImageCaptured method in the AF_Additions class
 * 
 * @param user_data : Standard glib function parameter, used to pass a pointer to th AF_Additions object
 */
gboolean AdditionsParent::focusImageCapturedWrapper(GstElement* fsink,
    GstBuffer* buffer, GstPad* pad, gpointer user_data){
        return reinterpret_cast<AF_Additions*>(&af_iface_)->focusImageCaptured(fsink,
        buffer, pad, &af_iface_);
}

/**
 * This communicates errors in the C++ to nvgstcapture-1.0 and forces shutdown
 * This function calls the stored function pointer to the base nvgstcapture-1.0
 * error handling routine.
 * @param error : Pointer to captured error data
 */
void AdditionsParent::errorShutdown(GError** error){
    error_ = error;
    additions_exit_capture_(error_);
}

/**
 * Focus image capture control; capture image frames.
 * This function calls the stored function pointer to the base nvgstcapture-1.0
 * routine to allow a focus frame to pass through.
 */
void AdditionsParent::openFocusValve(){
    focus_valve_open_();
}

/**
 * Focus image capture control; block image frames from being captured.
 * This function calls the stored function pointer to the base nvgstcapture-1.0
 * routine to stop focus frames passing through.
 */
void AdditionsParent::closeFocusValve(){
    focus_valve_close_();
}

/**
 * Capture the current image capture resolution for use with the C++ additions
 * 
 * @param * wide : A gint pointer to store the resolution width value
 * @param * high : A gint pointer to store the resolution height value
 */
void AdditionsParent::getResolution(gint* wide, gint* high){

    *wide = *width_;
    *high = *height_;
}

/**
 * Use a function pointer to trigger image capture in nvgstcapture-1.0
 * This function calls the stored function pointer to the base nvgstcapture-1.0
 * routine to trigger an image capture.
 * 
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 */
gboolean AdditionsParent::timeoutTriggerCallback(gpointer user_data) {
    AdditionsParent* self = static_cast<AdditionsParent*>(user_data);
    
    self->trigger_image_capture_();

    return G_SOURCE_CONTINUE;
}

/**
 * The functions below are made accessible to be called from the C coded nvgstcapture-1.0 main application.
 * These functions provide the main nvgstcapture-1.0 application access to the routines it needs in its operation.
 */
extern "C" {
    /**
    * Interface function to create the AdditionsParent Object in nvgstcapture-1.0
    * @param main_context : A string identifier for the camera to be controlled through I2C.
    * @param * width : A pointer the curent resolution width in nvgstcapture-1.0
    * @param * height : A pointer the curent resolution height in nvgstcapture-1.0
    * @param trigger_image_capture : A function pointer to an nvgstcapture-1.0 function to
    *                                trigger image capture.
    * @param additions_exit_capture : A function pointer to an nvgstcapture-1.0 function to
    *                                 trigger application exit due to error.
    * @param focus_valve_open :  A function pointer to an nvgstcapture-1.0 function to
    *                            allow focus frames to pass through.
    * @param focus_valve_close :  A function pointer to an nvgstcapture-1.0 function to
    *                             stop focus frames passing through.
    * @param error : Pointer the nvgstcapture-1.0 error struct for error reporting
    * 
    * @return : A pointer to an AdditionsParent Object
    */
    AdditionsParent* additions_parent_create(GMainContext* main_context, gint* width, gint* height,
    TriggerImageCapture trigger_image_capture, AdditionsExitCapture additions_exit_capture,
    FocusValveOpen focus_valve_open, FocusValveClose focus_valve_close, GError** error) {
        g_print ("\nCreating additional objects\n"); //Log object construction to the terminal
        g_print ("...Additions parent\n");
        return new AdditionsParent(main_context, width, height, trigger_image_capture,
            additions_exit_capture, focus_valve_open, focus_valve_close, error);
    }

    /**
    * Interface function to destroy the AdditionsParent Object in nvgstcapture-1.0
    */
    void additions_parent_destroy(AdditionsParent* obj) {
        delete obj;
    }

    /**
    * Interface function to setup the AdditionsParent Object, and its dependants after the camera has come online
    * 
    * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
    */
    gboolean runSetup_C(gpointer user_data) {
        AdditionsParent* obj = static_cast<AdditionsParent*>(user_data);
        obj->runSetupWrapper(obj);
        return FALSE; //This will make it run only once
    }
    
    /**
    * Interface function to pass the focus image buffer for AF processing
    * 
    * @param * fsink : Pointer to the  fsink object in nvgstcapture-1.0
    * @param * buffer : Pointer to the gstreamer image buffer in nvgstcapture-1.0
    * @param * pad : Pointer to the gstreamer pad object in nvgstcapture-1.0
    * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
    * 
    * @return : gboolean value passed through from focusImageCaptured via
    * focusImageCapturedWrapper
    */
    gboolean focusImageCaptured_C(GstElement* fsink,
        GstBuffer* buffer, GstPad* pad, gpointer user_data){
            AdditionsParent* obj = static_cast<AdditionsParent*>(user_data);
            return obj->focusImageCapturedWrapper(fsink,
            buffer, pad, obj);
    }

    /**
    * Interface function to get the required filename from the C++ extensions
    * 
    * @param : * obj: point to the AdditionsParent object
    * @param * outfile: pointer to the new filename string
    * 
    * @return : gboolean value passed through from getImageFileName
    */
    void getImageFileName_C(AdditionsParent* obj, char* outfile) {
        return obj->output_file_control_.getImageFileName(outfile);
    }

} //extern "C"
        

