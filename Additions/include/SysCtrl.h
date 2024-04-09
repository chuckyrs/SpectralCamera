#ifndef SYSCTRL_H
#define SYSCTRL_H

#include <glib.h>

#include "JetsonNanoGPIO.h"
#include "SerialIO.h"
#include "amsAS7265x.h"

class AdditionsParent;
class ErrorHandler;
class OutputFileControl;

class SysCtrl {
public:
 
    GSourceFunc releaseFocusLock;

    SysCtrl(GMainContext* main_context,
            AdditionsParent* additions_parent,
            OutputFileControl* output_file_control,
            ErrorHandler* error_handler);

    ~SysCtrl();
    gint setup(GError** error);
    void run_ams7265xHandshake();  
    void setFocusLock(gboolean value);
    gboolean getFocusLock();

    //static gboolean GPIO_InputPinChangeWrapper(gpointer data);
    static gboolean GPIO_LightsOutWrapper(gpointer user_data);
    static gboolean GPIO_AmbientOnWrapper(gpointer user_data);
    static gboolean GPIO_FlashOnWrapper(gpointer user_data);

    void GPIO_InputPinChange();
    gboolean GPIO_LightsOut(gpointer user_data);
    gboolean GPIO_AmbientOn(gpointer user_data);
    gboolean GPIO_FlashOn(gpointer user_data);

private:
    GMainContext* main_context_;

    gboolean focusLock;

    //Pointed objects
    AdditionsParent* additions_parent_;
    ErrorHandler* error_handler_;
    OutputFileControl* output_file_control_;

    //My objects
    SerialPort usb0_serial_port_;
    AS7265xUnit as7265x_unit_;
    GPIO_InputPin input_pin_7_; //Offset 216
    GPIO_OutputPin output_pin_38_; //Offset 77
    GPIO_OutputPin output_pin_40_; //Offset 78
    

    //Own objects
    //GPIO_Output_pin ambientLED_CtrlObj;
    //GPIO_Output_pin testLED_CtrlObj;
    //GPIO_Input_Pin_Polling buttonResponseObj;
    //SerialPort SerialPortObj;
    //AS7265xUnit AS7265xUnitObj;
};

#endif  // SYSCTRL_H