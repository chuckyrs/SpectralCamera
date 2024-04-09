#ifndef AS7265X_H
#define AS7265X_H

#include <glib.h>
#include <linux/gpio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <memory>


#include "OutputFileControl.h"
#include "SerialIO.h"

class OutputFileControl;
class ErrorHandler;

class AS7265xUnit {
public:
    AS7265xUnit(SerialPort* serial_port, OutputFileControl* file_to_write, ErrorHandler* error_handler);
    ~AS7265xUnit();    
    void getHandshakeData();
    static gboolean getAS7265xDataWrapper(gpointer user_data);
    gboolean getAS7265xData(void);

private:
    OutputFileControl* output_file_;
    SerialPort* serial_port_;
    ErrorHandler* error_handler_;

    std::vector<std::string> raw_tokens_;
    std::vector<std::string> calibrated_tokens_;
    std::vector<int> order_ = { 8, 10, 12, 13, 14, 15, 6, 7, 9, 11, 16, 17, 0, 1, 2, 3, 4, 5 };
    std::vector<int> channels_ = { 610, 680, 730, 760, 810, 860, 560, 585, 645, 705, 900, 940, 410, 435, 460, 485, 510, 535 };

    guint sequence_no_;    

    //readyToRun();
    std::vector<std::string> split(const std::string& s, char delimiter);
    void runHandshake(GError** error);
    void runData(GError** error);
    //static void handshakeReplyStatic(const std::string& output_data);
    void handshakeReply(const std::string& output_data);
    void dataReply(const std::string& output_data);
};
#endif
