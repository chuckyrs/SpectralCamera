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
};

#endif  // SYSCTRL_H