#ifndef OUTPUTFILECONTROL_H
#define OUTPUTFILECONTROL_H

#include <glib.h>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>
#include <dirent.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <memory>

#define LINE_FEED 0x0A

class ErrorHandler;

class OutputFileControl {
public:
    OutputFileControl(const std::string& path_root, ErrorHandler* error_handler);
    ~OutputFileControl();
    gint setup(GError** error);

    gint writeDataFileTime(GError** error);
    gint writeLineToFile(const std::string& data, GError** error);
    void getImageFileName(char* outfile);  
    std::string getNextFilename(GError** error);
    void captureDataTime();
    void setButtonTriggered();
    
private:
    ErrorHandler* error_handler_;
    std::string path_root_;
    std::string data_time_;
    std::string daily_dir_;

    gboolean button_triggered_;

    GIOChannel* output_file_channel_;

    void createDailyDir();
    void freeDailyDir();
    void freeDataTime();
};


#endif  // OUTPUTFILECONTROL_H