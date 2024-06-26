#include <sys/stat.h>
#include <sstream>
#include <iomanip>
#include <memory>
#include <cstring>

#include "OutputFileControl.h"
#include "ErrorHandler.h"

OutputFileControl::OutputFileControl(const std::string& path_root, ErrorHandler* error_handler):
    path_root_(path_root), output_file_channel_(nullptr), daily_dir_(""),
    data_time_(""), button_triggered_(FALSE),
    error_handler_(error_handler) {

        g_print("...Output file controller\n");

        //g_print("OutputFileControl: %s %s", daily_dir_.c_str(), data_time_.c_str());
}

OutputFileControl::~OutputFileControl() {
    g_print("Shuting down output file controller\n");
    freeDataTime();
    freeDailyDir();
    // Close the file
    g_io_channel_shutdown(output_file_channel_, TRUE, NULL);
    g_io_channel_unref(output_file_channel_);
    g_print("Output file closed\n");
}

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

gint OutputFileControl::writeDataFileTime(GError** error) {

    return writeLineToFile(data_time_, error);
}

void OutputFileControl::setButtonTriggered() {
    button_triggered_= TRUE;
}

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

void OutputFileControl::freeDataTime() {
    data_time_.clear();
}

/*std::string OutputFileControl::getDataTime() {

    if (!data_time.empty())
        return data_time;
    /*else
    * {
    * ERROR HERE
    * }
}*/

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

void OutputFileControl::freeDailyDir() {
    daily_dir_.clear();
}

/*std::string OutputFileControl::get_daily_dir() {
    return daily_dir;
}*/

