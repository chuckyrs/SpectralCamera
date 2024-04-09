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

/*Init_Additions Class Definition*/
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

AdditionsParent::~AdditionsParent(){
    g_print("Closing and removing all additional objects...\n");

}

void AdditionsParent::runSetupWrapper(gpointer user_data) {
        reinterpret_cast<AdditionsParent*>(user_data)->setup(user_data);
}

void AdditionsParent::setup(gpointer user_data) {
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

gboolean AdditionsParent::focusImageCapturedWrapper(GstElement* fsink,
    GstBuffer* buffer, GstPad* pad, gpointer user_data){
        return reinterpret_cast<AF_Additions*>(&af_iface_)->focusImageCaptured(fsink,
        buffer, pad, &af_iface_);
}

void AdditionsParent::errorShutdown(GError** error){
    error_ = error;
    additions_exit_capture_(error_);
}

void AdditionsParent::openFocusValve(){
    focus_valve_open_();
}

void AdditionsParent::closeFocusValve(){
    focus_valve_close_();
}

void AdditionsParent::getResolution(gint* wide, gint* high){

    /**************Check for correctness*****************/
    *wide = *width_;
    *high = *height_;
}

void AdditionsParent::callMeFrom_C() {
    g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, 30, timeoutTriggerCallback, this, nullptr);
}

gboolean AdditionsParent::timeoutTriggerCallback(gpointer user_data) {
    AdditionsParent* self = static_cast<AdditionsParent*>(user_data);
    // Access self->main_context_ and self->cam_ctx_ here for further use
    //NVGST_ERROR_MESSAGE ("This does not compile");

    self->trigger_image_capture_();

    return G_SOURCE_CONTINUE;
}

extern "C" {

AdditionsParent* additions_parent_create(GMainContext* main_context, gint* width, gint* height,
TriggerImageCapture trigger_image_capture, AdditionsExitCapture additions_exit_capture,
FocusValveOpen focus_valve_open, FocusValveClose focus_valve_close, GError** error) {
    g_print ("\nCreating additional objects\n");
    g_print ("...Additions parent\n");
    return new AdditionsParent(main_context, width, height, trigger_image_capture,
        additions_exit_capture, focus_valve_open, focus_valve_close, error);
}

void additions_parent_destroy(AdditionsParent* obj) {
    delete obj;
}

gboolean focusImageCaptured_C(GstElement* fsink,
    GstBuffer* buffer, GstPad* pad, gpointer user_data){
        AdditionsParent* obj = static_cast<AdditionsParent*>(user_data);
        return obj->focusImageCapturedWrapper(fsink,
        buffer, pad, obj);
}

gboolean runSetup_C(gpointer user_data) {
    AdditionsParent* obj = static_cast<AdditionsParent*>(user_data);
    obj->runSetupWrapper(obj);
    return FALSE; //This will make it run only once
}


void getImageFileName_C(AdditionsParent* obj, char* outfile) {
    return obj->output_file_control_.getImageFileName(outfile);
}

/*void additions_parent_callMeFrom_C(AdditionsParent* obj) {
    obj->callMeFrom_C();*/
}
        
        
        /*return reinterpret_cast<AF_Additions*>(user_data)->focusImageCaptured(fsink,
        buffer, pad, user_data);*/
//}

