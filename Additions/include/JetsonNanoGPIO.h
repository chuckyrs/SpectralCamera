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