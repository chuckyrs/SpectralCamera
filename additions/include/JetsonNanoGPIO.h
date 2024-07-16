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

#ifndef JETSONNANOGPIO_H
#define JETSONNANOGPIO_H

#include <glib.h>
#include <functional>
#include <linux/gpio.h>

class ErrorHandler;

class GPIO_OutputPin {
public:
    GPIO_OutputPin(gint GPIO_pin_number);
    ~GPIO_OutputPin();
    gint setup(GError** error);
    gint set(guint value, GError** error);
    gint get(GError** error);
    gboolean startedOK();

private:
    gboolean started_;
    gint GPIO_pin_number_;
    gint pin_offset_;
    gint GPIO_output_pin_fd_;
    struct gpiohandle_request req_;
    struct gpiohandle_data data_;
};

class GPIO_InputPin {
public:
    GPIO_InputPin(guint GPIO_pin_number, guint debounce_time,
    guint event_flag, ErrorHandler* error_handler);
    ~GPIO_InputPin();
    void setPinCallbackFunction(std::function<void()> func);
    void unsetPinCallbackFunction(std::function<void()> func);

    gint setup(GError** error);
    static gboolean inputChangeWrapper(GIOChannel* src_io_channel, GIOCondition cond, gpointer data);
    static gboolean cancelDebounceWrapper(gpointer user_data);


private:
    ErrorHandler* error_handler_;
    GIOChannel* input_pin_channel_;
    struct gpioevent_request rq_ = {0};
    guint callback_handler_in_;
    gboolean callback_activated_;
    gboolean button_debounce_;
    gint pin_number_;
    gint pin_offset_;
    guint debounce_timeout_;
    guint event_flag_;

    std::function<void()> pinFunc_;// = NULL;
    void pinCallbackFunction();
    void closePin();
    gboolean inputChangeInstance(GIOChannel* src_io_channel, GIOCondition cond, gpointer data);
    gboolean cancelDebounceInstance(gpointer user_data);
};

#endif //JETSONNANOGPIO_H