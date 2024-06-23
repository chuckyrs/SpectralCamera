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
 * Constructs a CameraI2CDevice object associated with a specific camera using its identifier.
 *
 * @param camera_id : A string identifier for the camera to be controlled through I2C.
 */
AdditionsParent::AdditionsParent(GMainContext* main_context, gint* width, gint* height,
    TriggerImageCapture trigger_image_capture, AdditionsExitCapture additions_exit_capture,
    FocusValveOpen focus_valve_open, FocusValveClose focus_valve_close, GError** error) 
    : main_context_(main_context), width_(width), height_(height), trigger_image_capture_(trigger_image_capture),
    additions_exit_capture_(additions_exit_capture), focus_valve_open_(focus_valve_open),
    focus_valve_close_(focus_valve_close), error_(error),
    error_handler_(this),
    output_file_control_("/home/chuck/New_Data/", &error_handler_),
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
 * This wrapper reinterprets the gpointer user_data object into usable pointer for accessing
 * the setup method in the AdditionsParent class 
 */
void AdditionsParent::runSetupWrapper(gpointer user_data) {
        reinterpret_cast<AdditionsParent*>(user_data)->setup(user_data);
}

/**
 *
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
 */
gboolean AdditionsParent::focusImageCapturedWrapper(GstElement* fsink,
    GstBuffer* buffer, GstPad* pad, gpointer user_data){
        return reinterpret_cast<AF_Additions*>(&af_iface_)->focusImageCaptured(fsink,
        buffer, pad, &af_iface_);
}

/**
 *This communicates errors in the C++ to nvgstcapture-1.0 and forces shutdown
 */
void AdditionsParent::errorShutdown(GError** error){
    error_ = error;
    additions_exit_capture_(error_);
}

/**
 *Focus image capture control; capture image frames
 */
void AdditionsParent::openFocusValve(){
    focus_valve_open_();
}

/**
 *Focus image capture control; block image frames from being captured
 */
void AdditionsParent::closeFocusValve(){
    focus_valve_close_();
}

/**
 *Capture the current image capture resolution for use with the C++ additions
 */
void AdditionsParent::getResolution(gint* wide, gint* high){

    *wide = *width_;
    *high = *height_;
}

/**
 *Use a function pointer to trigger image capture in nvgstcapture-1.0
 */
gboolean AdditionsParent::timeoutTriggerCallback(gpointer user_data) {
    AdditionsParent* self = static_cast<AdditionsParent*>(user_data);
    
    self->trigger_image_capture_();

    return G_SOURCE_CONTINUE;
}

/**
 *
 */
extern "C" {
    /**
    *Interface function to create the AdditionsParent Object in nvgstcapture-1.0
    * 
    * Returns a pointer to an AdditionsParent Object
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
    *Interface function to destroy the AdditionsParent Object in nvgstcapture-1.0
    */
    void additions_parent_destroy(AdditionsParent* obj) {
        delete obj;
    }

    /**
    *Interface function to setup the AdditionsParent Object, and its dependants after the camera has come online
    */
    gboolean runSetup_C(gpointer user_data) {
        AdditionsParent* obj = static_cast<AdditionsParent*>(user_data);
        obj->runSetupWrapper(obj);
        return FALSE; //This will make it run only once
    }
    
    /**
    *Interface function to pass the focus image buffer for AF processing
    */
    gboolean focusImageCaptured_C(GstElement* fsink,
        GstBuffer* buffer, GstPad* pad, gpointer user_data){
            AdditionsParent* obj = static_cast<AdditionsParent*>(user_data);
            return obj->focusImageCapturedWrapper(fsink,
            buffer, pad, obj);
    }

    /**
    *Interface function to get the required filename from the C++ extensions
    */
    void getImageFileName_C(AdditionsParent* obj, char* outfile) {
        return obj->output_file_control_.getImageFileName(outfile);
    }

} //extern "C"
        

