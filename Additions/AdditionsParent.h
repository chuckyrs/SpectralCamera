#ifndef ADDITIONSPARENT_H
#define ADDITIONSPARENT_H

#include "AdditionsParent_C.h"
#include "SysCtrl.h"
#include "OutputFileControl.h"
#include "ErrorHandler.h"
#include "AdditionsForAF.h"

class AdditionsParent {
public:

    AdditionsParent(GMainContext* main_context, gint* width, gint* height,
        TriggerImageCapture trigger_image_capture,
        AdditionsExitCapture additions_exit_capture, FocusValveOpen focus_valve_open,
        FocusValveClose focus_valve_close, GError** error);
    ~AdditionsParent();
    void runSetupWrapper(gpointer user_data);
    void errorShutdown(GError** error);
    void openFocusValve();
    void closeFocusValve();
    void getResolution(gint* wide, gint* high);
    gboolean focusImageCapturedWrapper(GstElement* fsink,
    GstBuffer* buffer, GstPad* pad, gpointer user_data);
    void callMeFrom_C();
    static gboolean timeoutTriggerCallback(gpointer user_data);

    //Owned Objects
    ErrorHandler error_handler_;
    OutputFileControl output_file_control_;
    SysCtrl system_control_;
    AF_Additions af_iface_;
     // static wrappers
    
    //I2CsetFocus I2CsetFocusObj;
    //Autofocus in here - Probably a better fit. Might need a routine here to watch camera context for running state.
    

private:
    GError** error_;
    gint* width_;
    gint* height_;
    GMainContext* main_context_;
    gboolean error_during_setup_;
    TriggerImageCapture trigger_image_capture_;  // storing the function pointer
    AdditionsExitCapture additions_exit_capture_;
    FocusValveOpen focus_valve_open_;
    FocusValveClose focus_valve_close_; 

    void setup(gpointer user_data);
};
#endif  // ADDITIONSPARENT_H