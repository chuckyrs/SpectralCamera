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

#include "SerialIO.h"
#include "JetsonNanoMaps.h"
#include "ErrorHandler.h"

/**
 * Constructs a SerialIO object associated with a serial port.
 * This handles sending and receivijng data at the port.
 *
 * @param port_id : The UART port to associte with this object.
 * @param baud : Port baud rate setting
 * @param bits : Port no. of bits setting
 * @param stopBits : Port no. of stop bits setting
 * @param parity : Port parity setting
 * @param flowControl : Port flow control setting
 * @param * error_handler : Pointer to the application's error_handler object
 * 
 */
SerialPort::SerialPort(const std::string& port_id, guint baud, guint bits, guint stopBits,
    guint parity, guint flowControl, ErrorHandler* error_handler)
    : port_(port_id), baud_(baud), bits_(bits), stop_bits_(stopBits),parity_(parity),
    flow_control_(flowControl), serial_port_fd_(-1), callback_activated_(FALSE), buffer_write_pos_(nullptr),
    in_buffer_position_(0), disable_port_lock_(FALSE), writeFunc_(nullptr), write_func_set_(FALSE),
    error_handler_(error_handler) { 

    g_print ("...Serial port controller on %s\n", port_id.c_str());
}

/**
 * Destructor for SerialPort. Logs the shutdown process and cleans up resources.
 */
SerialPort::~SerialPort() {
    g_print("Shutting down serial port on %s\n", port_.c_str());   
    closePort();
    g_print("Serial port closed\n");
}

/**
 * Open and setup the serial port with the object settings.
 * 
 * @param error : Pointer the nvgstcapture-1.0 error struct for error reporting
 * 
 * @return : -1 on error, otherwise 0.
 */
gint SerialPort::setup(GError** error) {

    std::string port_id = port_;
    JetsonNanoDeviceMap portIdToDevice;
    port_.clear();

    port_ = portIdToDevice.identifierToDevice(port_id, error);

    if (port_.empty()) //Error should be set
        return -1;
    g_print("Setting up serial port '%s' (%s)\n", port_id.c_str(), port_.c_str());

    gint ret_val = configurePort(error); 

    if (ret_val == -1) //Error should be set by here as well
        g_print("Error opening serial port on %s \n", port_.c_str());
    else
        g_print("Serial port %s open\n", port_id.c_str()); 

    return ret_val; 
}

/**
* This sets the port incomming data handling function. This can be changed as required.
* 
* @param func : The data handling function to bind as the write function.
*/
void SerialPort::setWriteFunc(std::function<void(const std::string&)> func) {
        writeFunc_ = func;
        write_func_set_ = TRUE;
}

/**
* This unsets the port incomming data handling function. Incomming data will be unhandled.
* 
* @param func : Any dummy function, this is a work around. Ulitimately we will have a null pointer.
*/
void SerialPort::unsetWriteFunc(std::function<void(const std::string&)> func) {
        writeFunc_ = nullptr;
        write_func_set_ = FALSE;
}

/**
* This writes charaters for output via the io channel, and thus the port.
* 
* @param string_to_send : The string for the port to output.
* @param error : Pointer the nvgstcapture-1.0 error struct for error reporting
* @return : -1 on error, otherwise 0.
*/
gint SerialPort::sendChars(const std::string& string_to_send, GError** error) {
    gsize bytes_written = 0;
    gssize count;
    std::string string_with_linefeed = string_to_send + static_cast<char>(LINE_FEED);
    GIOStatus status;

    //Normally it never happens, but it is better not to segfault ;) 
    if ((serial_port_fd_ == -1) || (string_to_send.length() == 0)) {

        g_set_error_literal(error, g_quark_from_static_string("serial device"),1,
        "String length zero, or file descriptor closed");
        return -1;
    }
       
    count = string_with_linefeed.size();
   
    status = g_io_channel_write_chars(static_channel, (gchar*)string_with_linefeed.c_str(), count, &bytes_written, error);

    if (status == G_IO_STATUS_ERROR){
        //error should already be set
        return -1;
    }

    status = g_io_channel_flush(static_channel, error);

    if (status == G_IO_STATUS_ERROR){
        //error should already be set
        return -1;
    }

    return bytes_written;
}

/**
* Incommming data handling function called from listenPort.
* 
* @param output_chars : The string to send to the currently bound data handling function.
*/
void SerialPort::writeCharsToFunc(const std::string& output_chars) {
    if ((write_func_set_) && (!output_chars.empty())) {    
        writeFunc_(output_chars);
    }
}

/**
 * CALLBACK FUNCTION. Called when data arrives at the port to handle the data. This wrapper reinterprets the
 * gpointer user_data object into usable pointer for accessing the setup method in the AdditionsParent class.
 *
 * @param * src_io_channel : The io channel that received the data
 * @param cond : Triggered channel condition
 * @param data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 * 
 * @return : gboolean value passed through from listenPort
 */
gboolean SerialPort::listenPortStatic(GIOChannel* src_io_channel, GIOCondition cond, gpointer data) {
    return reinterpret_cast<SerialPort*>(data)->listenPort(src_io_channel, cond, data);
}

/**
 * CLASS METHOD. Called when data arrives at the port to handle the data. We static cast the user_data pointer to access its methods.
 *
 * @param * src_io_channel : The io channel that received the data
 * @param cond : Triggered channel condition
 * @param data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 *
 * @return : return TRUE will ensure port keeps listening for Data, return FALSE on Error detection.
 */
gboolean SerialPort::listenPort(GIOChannel* src_io_channel, GIOCondition cond, gpointer data) {
    SerialPort* self = static_cast<SerialPort*>(data);
    gsize len = 0;
    gsize buffer_size;
    gchar last_char;
    GError* error = nullptr;

    self->buffer_.resize(BUFFER_SIZE);
    buffer_size = self->buffer_.size() - self->in_buffer_position_;

    GIOStatus read_outcome = g_io_channel_read_chars(src_io_channel, self->buffer_.data() + self->in_buffer_position_, buffer_size, &len, &error);

    if (read_outcome == G_IO_STATUS_ERROR){
        //Assume error is set
        self->error_handler_->errorHandler(&error);

        return FALSE; //KILL THIS OFF HERE AS WE WILL EXIT ON ERROR
    }

    if(len > 0) {
        self->in_buffer_position_ += len;

        last_char = self->buffer_[self->in_buffer_position_ - 1];

        if ((last_char == LINE_FEED) || (last_char == CARRIAGE_RETURN)) { //Trim off the line feed here
            std::string buffer_to_string(buffer_.begin(), buffer_.begin() + (self->in_buffer_position_ - 1));

            writeCharsToFunc(buffer_to_string.c_str()); //These are the reply functions in amsAS7265x.cpp
            //Handle write errors there.

            self->buffer_.clear();
            self->in_buffer_position_ = 0;
        }   
    } 
    return TRUE; //ALWAYS RETRUN TRUE TO KEEP THIS ALIVE
}

/**
 * CALLBACK FUNCTION. Called when an error occurs in the io channel. This wrapper reinterprets the
 * gpointer user_data object into usable pointer for accessing the setup method in the AdditionsParent class.
 *
 * @param * src : The io channel that received the data
 * @param cond : Triggered channel condition
 * @param data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 * 
 * @return : gboolean value passed through from ioErr
 */
gboolean SerialPort::ioErrStatic(GIOChannel* src, GIOCondition cond, gpointer data) {
    return reinterpret_cast<SerialPort*>(data)->ioErr(src, cond, data);
}

/**
 * CLASS METHOD. Called when an error occurs in the io channel. We static cast the user_data pointer to access its methods.
 *
 * * @param * src : The io channel that received the data
 * @param cond : Triggered channel condition
 * @param data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 *
 * @return : return FALSE will close this channel.
 */
gboolean SerialPort::ioErr(GIOChannel* src, GIOCondition cond, gpointer data) {
        SerialPort* self = static_cast<SerialPort*>(data);

    self->closePort();
    return FALSE;
}



/**
* This method configures the serial port for use with the settings theis object was built with.
* 
* @param error : Pointer the nvgstcapture-1.0 error struct for error reporting.
* 
* @return : -1 on error, else 0.
*/
gint SerialPort::configurePort(GError** error) {
    struct termios termios_p;

    closePort();
    serial_port_fd_ = open(port_.c_str(), O_RDWR | O_NONBLOCK);

    if(serial_port_fd_ == -1)
    {
        g_set_error(error, g_quark_from_static_string("SerialIO port error"),1,
            "Can not open serial device '%s'", port_.c_str());
        return -1;
    }

    if(!disable_port_lock_)
    {
        if(flock(serial_port_fd_, LOCK_EX | LOCK_NB) == -1)
        {
        closePort();
         g_set_error(error, g_quark_from_static_string("SerialIO port error"),1,
            "Cannot lock port! Serial device '%s' may currently be in use by another program.'", port_.c_str());
        return -1;
        }
    }

    //Get the exisiting port settings!
    tcgetattr(serial_port_fd_, &termios_p);

    switch(baud_)
    {
    case 300:
        termios_p.c_cflag = B300;
        break;
    case 600:
        termios_p.c_cflag = B600;
        break;
    case 1200:
        termios_p.c_cflag = B1200;
        break;
    case 2400:
        termios_p.c_cflag = B2400;
        break;
    case 4800:
        termios_p.c_cflag = B4800;
        break;
    case 9600:
        termios_p.c_cflag = B9600;
        break;
    case 19200:
        termios_p.c_cflag = B19200;
        break;
    case 38400:
        termios_p.c_cflag = B38400;
        break;
    case 57600:
        termios_p.c_cflag = B57600;
        break;
    case 115200:
        termios_p.c_cflag = B115200;
        break;
    case 230400:
        termios_p.c_cflag = B230400;
        break;
    case 460800:
        termios_p.c_cflag = B460800;
        break;
    case 576000:
        termios_p.c_cflag = B576000;
        break;
    case 921600:
        termios_p.c_cflag = B921600;
        break;
    case 1000000:
        termios_p.c_cflag = B1000000;
        break;
    case 1500000:
        termios_p.c_cflag = B1500000;
        break;
    case 2000000:
        termios_p.c_cflag = B2000000;
        break;

    default:
#ifdef HAVE_LINUX_SERIAL_H
        set_custom_speed(config.baud, serial_port_fd);
        termios_p.c_cflag |= B38400;
#else
        closePort();
        return FALSE;
#endif
    }

    termios_p.c_cflag &= ~CSIZE; //This should clear all size bits

    switch(bits_)
    {
    case 5:
        termios_p.c_cflag |= CS5;
        break;
    case 6:
        termios_p.c_cflag |= CS6;
        break;
    case 7:
        termios_p.c_cflag |= CS7;
        break;
    case 8:
        termios_p.c_cflag |= CS8;
        break;
    }
    switch(parity_)
    {
    case 1:
        termios_p.c_cflag |= PARODD | PARENB;
        break;
    case 2:
        termios_p.c_cflag |= PARENB;
        break;
    default:
        termios_p.c_cflag &= ~PARENB;
        break;
    }
    if(stop_bits_ == 2)
        termios_p.c_cflag |= CSTOPB;
    else
        termios_p.c_cflag &= ~CSTOPB;

    termios_p.c_cflag |= CREAD;
    termios_p.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
    switch(flow_control_)
    {
    case 1:
        termios_p.c_iflag |= IXON | IXOFF;
        break;
    case 2:
        termios_p.c_cflag |= CRTSCTS;
        break;
    default:
        termios_p.c_iflag &= ~(IXON | IXOFF | IXANY);		
        termios_p.c_cflag &= ~CRTSCTS;
        termios_p.c_cflag |= CLOCAL;
        break;
    }
        
    termios_p.c_lflag &= ~ICANON;
    termios_p.c_lflag &= ~ECHO;
    termios_p.c_lflag &= ~ECHOE;
    termios_p.c_lflag &= ~ECHONL;
    termios_p.c_lflag &= ~ISIG;

    termios_p.c_oflag = 0;
    termios_p.c_lflag = 0;
    termios_p.c_cc[VTIME] = 0;
    termios_p.c_cc[VMIN] = 0;
    tcsetattr(serial_port_fd_, TCSANOW, &termios_p);
    tcflush(serial_port_fd_, TCOFLUSH);
    tcflush(serial_port_fd_, TCIFLUSH);

    static_channel = g_io_channel_unix_new(serial_port_fd_);

    g_io_channel_set_encoding(static_channel,NULL,NULL);

    if (static_channel)
    {
        

        callback_handler_in_ = g_io_add_watch_full(static_channel,
                        G_PRIORITY_HIGH,
                        G_IO_IN,
                        (GIOFunc)SerialPort::listenPortStatic,
                        this, nullptr);

        g_io_channel_unref(static_channel);

    }
    else
    {
        return -1;
    }
   
    GIOChannel* err_channel = g_io_channel_unix_new(serial_port_fd_);
    
    g_io_channel_set_encoding(err_channel,NULL,NULL);

    if (err_channel)
    {
        
        callback_handler_err_ = g_io_add_watch_full(err_channel,
                        10,
                        G_IO_ERR,
                        (GIOFunc)SerialPort::ioErrStatic,
                        this, nullptr);

        g_io_channel_unref(err_channel);
    }
    else
    {
        return -1;
    }         
        
    callback_activated_ = TRUE;
    return serial_port_fd_;
}

/**
* This method closes the serial port and restores its original settings.
*/
void SerialPort::closePort() {
    if(serial_port_fd_ != -1)
    {
        if(callback_activated_ == TRUE)
        {
            g_source_remove(callback_handler_in_);
            g_source_remove(callback_handler_err_);
            callback_activated_ = FALSE;
        }
        
        tcsetattr(serial_port_fd_, TCSANOW, &termios_save);
        tcflush(serial_port_fd_, TCOFLUSH);
        tcflush(serial_port_fd_, TCIFLUSH);
        if(!disable_port_lock_)
        {
            flock(serial_port_fd_, LOCK_UN);
        }
        close(serial_port_fd_);
        serial_port_fd_ = -1;
    }
}