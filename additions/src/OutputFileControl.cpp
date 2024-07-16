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

#include <sys/stat.h>
#include <sstream>
#include <iomanip>
#include <memory>
#include <cstring>

#include "OutputFileControl.h"
#include "ErrorHandler.h"

/**
 * Constructs an AF_Additions object associated with a specific camera using its identifier.
 *
 * @param * path_root : The path to where data directories and files are to be stored
 * @param * error_handler : Pointer to the application's error_handler object
 *
 */
OutputFileControl::OutputFileControl(const std::string& path_root, ErrorHandler* error_handler):
    path_root_(path_root), output_file_channel_(nullptr), daily_dir_(""),
    data_time_(""), button_triggered_(FALSE),
    error_handler_(error_handler) {

        g_print("...Output file controller\n");
}

/**
 * Destructor for OutputFileControl. Logs the shutdown process and cleans up resources.
 */
OutputFileControl::~OutputFileControl() {
    g_print("Shuting down output file controller\n");
    freeDataTime();
    freeDailyDir();
    // Close the file
    g_io_channel_shutdown(output_file_channel_, TRUE, NULL);
    g_io_channel_unref(output_file_channel_);
    g_print("Output file closed\n");
}

/**
 * Setup for OutputFileControl. Part of the heirachial setup chain that occurs
 * only after the camera has come online.
 *
 * @param error : Pointer to the nvgstcapture-1.0 error struct for error reporting
 *
 * @return : -1 on error, else 0.
 */
gint OutputFileControl::setup(GError** error) {

    createDailyDir();    
    
    std::ostringstream file_path;
    std::string next_filename = getNextFilename(error);

    if (next_filename.empty())
        return -1; //Error should be set    
   
    file_path << daily_dir_<< next_filename.c_str();
    
    //Needs more ERROR HANDLING work here for setup
    // Open the file for writing. If it exists, it is truncated to 0 length.
    output_file_channel_ = g_io_channel_new_file(file_path.str().c_str(), "w", error);

    // Handle any errors
    if (output_file_channel_ == NULL) {
        g_set_error_literal(error, g_quark_from_static_string("Output file control"),1,
        "Could not open an outuput file channel");
        return -1;
    }

    g_print("Output file control setup... ");

    g_print ("The output file is: '%s'\n", file_path.str().c_str());
    return 0;
}

/**
* Writes a string of data to the currently open file.
* 
* @param data : The string data to write to the file.
* @param error : Pointer to the nvgstcapture-1.0 error struct for error reporting
* 
* @return : -1 on error, otherwise the number of bytes written
*/
gint OutputFileControl::writeLineToFile(const std::string& data, GError** error) {
    std::size_t bytes_written = 0;
    std::string data_with_linefeed = data + static_cast<char>(LINE_FEED);
    GIOStatus status;
    
    //Check if output_file_channel is empty.
    if (output_file_channel_ == NULL){
        g_set_error_literal(error, g_quark_from_static_string("File writing"),1,
        "The outuput file channel is not open");
        return -1;
    }

    //Check if the string is empty 
    if (data.length() == 0) 
        data_with_linefeed = static_cast<char>(LINE_FEED);
    else
        data_with_linefeed = data + static_cast<char>(LINE_FEED);

    // Write the data to the file
    status = g_io_channel_write_chars(output_file_channel_, data_with_linefeed.c_str(), -1, &bytes_written, error);

    if (status == G_IO_STATUS_ERROR){
        //error should already be set
        return -1;
    }

    status + g_io_channel_flush(output_file_channel_, error);

    if (status == G_IO_STATUS_ERROR){
        //error should already be set
        return -1;
    }

    
    return bytes_written;
}

/**
* This method is tied to the use of this camera and it makes a sequence of files oreder by their
* capture sequence. The number of the image capture aligns with the spectral data capture.
* 
* @param error : Pointer to the nvgstcapture-1.0 error struct for error reporting
* 
* @return : -1 on error, otherwise the number of bytes written
*/
std::string OutputFileControl::getNextFilename(GError **error) {
    int max_num = -1;

    DIR* dirp = opendir(daily_dir_.c_str());
    if (dirp == NULL) {
        g_set_error_literal(error, g_quark_from_static_string("Output file control"), 100, "Failed to open directory");
        return "";
    }

    struct dirent* dp;
    while ((dp = readdir(dirp)) != NULL) {
        std::string filename(dp->d_name);

        // Check if the filename starts with "AS7265x_data_" and ends with ".txt"
        if (filename.rfind("AS7265x_data_", 0) == 0 && filename.substr(filename.size() - 4) == ".txt") {
            try {
                // Get the number part of the filename
                int num = std::stoi(filename.substr(13, filename.size() - 17));

                // If this number is greater than the current maximum, update the maximum
                if (num > max_num) {
                    max_num = num;
                }
            } catch (std::exception& e) {
                g_set_error_literal(error, g_quark_from_static_string("Output file control"), 100, e.what());
                closedir(dirp); //"Could not create next file in sequence"
                return "";
            }
        }
    }
    closedir(dirp);

    // If no file was found in the directory, start numbering from 00
    if (max_num == -1) {
        max_num = 0;
    } else {
        // If files were found, increment the max number for the next filename
        max_num++;
    }

    // Prepare the next filename
    std::ostringstream next_filename;
    next_filename << "/AS7265x_data_"
        << std::setw(2) << std::setfill('0') << max_num
        << ".txt";

    return next_filename.str();
}

/**
* Captures the button press time for use in writing to files.
* The OutputFileControl private class property data_time_ is set by this method.
*/
void OutputFileControl::captureDataTime() {
    std::ostringstream temp_string;    
    
    if (!data_time_.empty())
        data_time_.clear();

    time_t T = time(NULL);
    struct tm tm = *localtime(&T);    

    temp_string << std::setw(2) << std::setfill('0') << tm.tm_hour << "-"
        << std::setw(2) << std::setfill('0') << tm.tm_min << "-"
        << std::setw(2) << std::setfill('0') << tm.tm_sec;

    data_time_ = temp_string.str();
}

/**
* Write the time data capture commenced to the file
* @param error : Pointer to the nvgstcapture-1.0 error struct for error reporting
* 
* @return : pass through from writeLineToFile()
*/
gint OutputFileControl::writeDataFileTime(GError** error) {

    return writeLineToFile(data_time_, error);
}

/**
* The OutputFileControl private class property button_triggered_ is set by this method.
* This allows us to track whether image capture was button activated, in which case spectral
* data is collected as well. Or whether the system was command activated, only capturing an image.
*/
void OutputFileControl::setButtonTriggered() {
    button_triggered_= TRUE;
}

/**
* Sets the image capture filename to the time of capture with a .jpg extension
* @ param outfile : This is a pointer to the filename buffer in nvgstcapture-1.0. We overwrite the
*                   default filename with the time of capture when image capture is triggered by the 
*                   button press.
*/
void OutputFileControl::getImageFileName(char* outfile) {
    std::ostringstream temp_string;

    if (button_triggered_){ //Only over write original filename if button triggered
        button_triggered_ = FALSE; //Button triggered is a one shot deal
        std::memset(outfile, '\0', 100); //Because we know the declaration of outfile is outfile[100]
        temp_string << daily_dir_ << data_time_ << ".jpg";

        //We now know the outfile memory space is full of nulls, so string less than 100 long
        //must be null terminated. This will work for me.
        std:strncpy(outfile, temp_string.str().c_str(), temp_string.str().size());

        //At this point the original filename is over written.
    }
}

/**
* The OutputFileControl private class property data_time_ is cleared by this method.
*/
void OutputFileControl::freeDataTime() {
    data_time_.clear();
}

/**
* Create a daily directory to store today's data
* 
* The private property daily_dir_ is set by this method
*/
void OutputFileControl::createDailyDir() {
    time_t T = time(NULL);
    struct tm tm = *localtime(&T);
    struct stat st = {0};
    std::ostringstream temp_string;

    if(!daily_dir_.empty())
        daily_dir_.clear();

    temp_string << path_root_.c_str() << tm.tm_year + 1900 << "-"
                << std::setw(2) << std::setfill('0') << tm.tm_mon + 1 << "-"
                << std::setw(2) << std::setfill('0') << tm.tm_mday;
    daily_dir_ = temp_string.str();

    g_print("The daily_dir is: '%s'... ", daily_dir_.c_str());

    if (stat(daily_dir_.c_str(), &st) == -1) {
        mkdir(daily_dir_.c_str(), 0777);
    }
}

/**
* The OutputFileControl private class property daily_dir_ is cleared by this method.
*/
void OutputFileControl::freeDailyDir() {
    daily_dir_.clear();
}

