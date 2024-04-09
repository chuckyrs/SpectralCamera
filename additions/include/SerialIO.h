#ifndef SERIALIO_H
#define SERIALIO_H

//#include <gtk/gtk.h>
#include <glib.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>
#include <string>
#include <pwd.h>
#include <vector>
#include <map>
#include <functional>

#define BUFFER_SIZE 256
#define LINE_FEED 0x0A
#define CARRIAGE_RETURN 0x0D

class ErrorHandler;

class SerialPort {
public:
    SerialPort(const std::string& device, guint baud, guint bits, guint stop_bits,
        guint parity, guint flow_control, ErrorHandler* error_handler);
    ~SerialPort();

    gint setup(GError** error);
    gint sendChars(const std::string& string_to_send, GError** error);
    void setWriteFunc(std::function<void(const std::string&)> func);
    void unsetWriteFunc(std::function<void(const std::string&)> func);

private:
    struct termios termios_save; //Save prior state to restore in closePort()
    GIOChannel* static_channel;
    ErrorHandler* error_handler_;

    gboolean write_func_set_;

    std::vector<gchar> buffer_;
    gchar *buffer_write_pos_;
    guint in_buffer_position_; 
    
    guint serial_port_fd_;
    guint callback_handler_in_, callback_handler_err_;
    gboolean callback_activated_;
           
    std::string port_;           // was gchar port[1024];
    guint baud_;                 //300 - 600 - 1200 - ... - 2000000
    guint bits_;                 // 5 - 6 - 7 - 8
    guint stop_bits_;             // 1 - 2
    guint parity_;               // 0 : None, 1 : Odd, 2 : Even
    guint flow_control_;          // 0 : None, 1 : Xon/Xoff, 2 : RTS/CTS, 3 : RS485halfduplex
    gboolean disable_port_lock_;

    std::function<void(const std::string&)> writeFunc_;// = NULL;
    // Static method
    static gboolean listenPortStatic(GIOChannel* src, GIOCondition cond, gpointer data);
    gboolean listenPort(GIOChannel* src, GIOCondition cond, gpointer data);

    // Static method
    static gboolean ioErrStatic(GIOChannel* src, GIOCondition cond, gpointer data);
    gboolean ioErr(GIOChannel* src, GIOCondition cond, gpointer data);

    gint configurePort(GError** error);
    void closePort();
    void writeCharsToFunc(const std::string& output_chars);
};

#endif  // SERIALIO_H
