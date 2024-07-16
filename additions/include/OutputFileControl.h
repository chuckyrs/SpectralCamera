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