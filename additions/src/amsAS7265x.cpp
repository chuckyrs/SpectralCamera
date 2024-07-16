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

#include "amsAS7265x.h"
#include "commands.h"
#include "ErrorHandler.h"

/**
 * Constructs an AS7265xUnit object configured with specified interfaces for communication and error handling.
 *
 * @param serial_port : Pointer to a SerialPort object used for I2C communication with the AS7265x sensor.
 * @param file_to_write : Pointer to an OutputFileControl object used for logging and data output.
 * @param error_handler : Pointer to an ErrorHandler object for managing error conditions.
 */
AS7265xUnit::AS7265xUnit(SerialPort* serial_port, OutputFileControl* file_to_write, ErrorHandler* error_handler) :
    sequence_no_(0), 
    serial_port_(serial_port), 
    output_file_(file_to_write),
    error_handler_(error_handler)  {
    
    g_print ("...AS7265x communications controller\n");
}

/**
 * Destructor for the AS7265xUnit. Logs the shutdown process and cleans up resources.
 */
AS7265xUnit::~AS7265xUnit(){
    g_print("Shutting down AS7265x communications controller\n");
    g_print("Nothing to do here\n"); // Indicates no explicit cleanup required (e.g., no dynamic memory allocation)
};

/**
 * Retrieves handshake data from the AS7265x device. This function abstracts the interaction process and manages errors.
 * This is a void function becuase there is no way to get a complete handshake before
 * the function returns. Any error will result in shutdown anyway.
 * 
 * @return gboolean : Always returns FALSE indicating a one-time operation per invocation.
 */

void AS7265xUnit::getHandshakeData() {
    GError* error = nullptr;

    g_print("Setting up AS7265x communications controller\n");
    g_print("Running handshake with AS7265x device...\n");
    
    //If there is an error in the handshake we'll let the error handler pick it up
    runHandshake(&error); // Starts the handshake process, passing the error pointer
    
}

/**
 * Wrapper function to facilitate data retrieval operations from a gpointer user data. This is typically used as a callback.
 *
 * @param user_data : Pointer to user_data expected to be a pointer to an instance of AS7265xUnit.
 * @return gboolean : Always returns FALSE as specified for this implementation.
 */
gboolean AS7265xUnit::getAS7265xDataWrapper(gpointer user_data){
    return reinterpret_cast<AS7265xUnit*>(user_data)->getAS7265xData();
}

/**
 * Retrieves data from the AS7265x device. This function abstracts the interaction process and manages errors.
 *
 * @return gboolean : Always returns FALSE indicating a one-time operation per invocation.
 */
gboolean AS7265xUnit::getAS7265xData(void)
{
    GError* error = nullptr;
    runData(&error);

    return FALSE;
}

/**
 * Splits a given string by a delimiter and returns the result as a vector of strings.
 *
 * @param s : The string to split.
 * @param delimiter : The character used as a delimiter to split the string.
 * @return std::vector<std::string> : A vector containing the split strings.
 */
std::vector<std::string> AS7265xUnit::split(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

/**
 * Conducts a multi-step handshake procedure with the AS7265x device, handling different commands in sequence.
 *
 * @param error : Pointer to a GError pointer, allowing error information to be updated and passed back.
 */
void AS7265xUnit::runHandshake(GError** error)
{
    std::string command;

    switch (sequence_no_)
    {
        case 0:
            command = AT_ACK;       
            serial_port_->setWriteFunc(std::bind(&AS7265xUnit::handshakeReply, this, std::placeholders::_1));
            if ((serial_port_->sendChars(command.c_str(), error)) == -1)
                goto error;
            break;

        case 1:
            command = AT_HARDWARE_VERSION;
            if ((serial_port_->sendChars(command.c_str(), error)) == -1)
                goto error;
            break;
        
        case 2:
            command = AT_SOFTWARE_VERSION;
            if ((serial_port_->sendChars(command.c_str(), error)) == -1)
                goto error;
            break;

        case 3:
            command = AT_SENSORS_PRESENT;
            if ((serial_port_->sendChars(command.c_str(), error)) == -1)
                goto error;            
            break;

        case 4:
            command = AT_SET_GAIN;
            if ((serial_port_->sendChars(command.c_str(), error)) == -1)
                goto error;            
            break;

        case 5:
            command = AT_SET_INTEGRATION_TIME;
            if ((serial_port_->sendChars(command.c_str(), error)) == -1)
                goto error;
            break;

        case 9:
            goto error;
    }

    return;

    error:

    error_handler_->errorHandler(error);

}

/**
 * Conducts a multi-step data retrieval procedure with the AS7265x device, handling different commands in sequence.
 *
 * @param error : Pointer to a GError pointer, allowing error information to be updated and passed back.
 */
void AS7265xUnit::runData(GError** error)
{
    switch (sequence_no_)
    {
        case 0:
            serial_port_->setWriteFunc(std::bind(&AS7265xUnit::dataReply, this, std::placeholders::_1));
            if ((serial_port_->sendChars(AT_SENSOR_TEMP, error)) == -1)
                goto error;          
            break;
    
        case 1:
            if ((serial_port_->sendChars(AT_GAIN, error)) == -1)
                goto error;         
            break;
    
        case 2:
            if ((serial_port_->sendChars(AT_INTEGRATION_TIME, error)) == -1)
                goto error;           
            break;
        
        case 3:
            if ((serial_port_->sendChars(AT_DATA, error)) == -1)
                goto error;         
            break;

        case 4:
            if ((serial_port_->sendChars(AT_CALIBRATED_DATA, error)) == -1)
                goto error;       
            break;

        case 9: goto error;

    }

    return;

    error:

    error_handler_->errorHandler(error);
}

/**
 * Conducts a multi-step handshake procedure with the AS7265x device, handling different commands in sequence.
 *
 * @param error : Pointer to a GError pointer, allowing error information to be updated and passed back.
 */
void AS7265xUnit::handshakeReply(const std::string& output_data)
{   GError* error = nullptr;
    std::ostringstream temp_data;   
    
    g_print("%s\n", output_data.c_str());
    switch(sequence_no_)
    {
        case 0:            
            temp_data << output_data.substr(0, output_data.size());
            sequence_no_++;
            runHandshake(&error);
            break;
    
        case 1:            
            temp_data << "AS7265x Hardware Version," << output_data.substr(0, output_data.size());
            if (output_file_->writeLineToFile(temp_data.str(), &error) == -1)
                goto error;
            sequence_no_++;
            runHandshake(&error);
            break;

        case 2:
            temp_data << "AS7265x Sofware Version," << output_data.substr(0, output_data.size());
            if (output_file_->writeLineToFile(temp_data.str(), &error) == -1)
                goto error;
            sequence_no_++;
            runHandshake(&error);
            break;

        case 3:      
            temp_data << "Sensors working," << output_data.substr(0, output_data.size() );
            if (output_file_->writeLineToFile(temp_data.str(), &error) == -1)
                goto error;
            sequence_no_++;
            runHandshake(&error);            
            break;
        case 4: //This is reply to ATGAIN=X command. Only receive an OK - nothing to save                
            sequence_no_++;
            runHandshake(&error);           
            break;
        case 5:  //This is reply to ATINTTIME=X command. Only receive an OK - nothing to save    
            sequence_no_=0; //Reset sequence for next shot
            serial_port_->unsetWriteFunc(std::bind(&AS7265xUnit::handshakeReply, this, std::placeholders::_1));

            //This will add a blank line at the end.
            if (output_file_->writeLineToFile("\n", &error) == -1)
                goto error;
            break;
    }

    return;
    
    error:
    sequence_no_= 9; //Use this number to deal with errors
    serial_port_->unsetWriteFunc(std::bind(&AS7265xUnit::handshakeReply, this, std::placeholders::_1));
    runHandshake(&error);
}

/**
 * Conducts a multi-step command sequence with the AS7265x to acquire spectral readings from the device.
 *
 * @param error : Pointer to a GError pointer, allowing error information to be updated and passed back.
 */
void AS7265xUnit::dataReply (const std::string& output_data)
{
    std::ostringstream temp_data;
    GError* error = nullptr;

    switch(sequence_no_)
    {
        case 0:
        {
            int temp_sensor = 1;
            std::istringstream ss(output_data);
            std::string token;

            //Start each new entry with the file time
            if (output_file_->writeDataFileTime(&error) == -1)
                goto error;

            //This writes each temperature sensor value to a new line in the file
            while (std::getline(ss, token, ',')) {

                std::ostringstream oss;
                oss << "Temp Sensor " << temp_sensor << "," << token;
                if (output_file_->writeLineToFile(oss.str(), &error) == -1){
                    goto error;
                    break;
                }
                temp_sensor++;
            }

            sequence_no_++;
            runData(&error);
            break;
        }
        
        case 1:
        {
            temp_data << "Sensor Gain," << output_data.substr(0, output_data.size() - 2);
            if (output_file_->writeLineToFile(temp_data.str(), &error) == -1)
                goto error;
            sequence_no_++;
            runData(&error);
            break;
        }
        
        case 2:
        {
            temp_data << "Sensor Integration Time," << output_data.substr(0, output_data.size() - 2);
            if (output_file_->writeLineToFile(temp_data.str(), &error) == -1)
                goto error;
            sequence_no_++;
            runData(&error);
            break;
        }
    
        case 3:
        {
            temp_data << "Channel, Raw Data, Calibrated Data";
            if (output_file_->writeLineToFile(temp_data.str(), &error) == -1)
                goto error;
            raw_tokens_ = split(output_data, ',');
            sequence_no_++;
            runData(&error);
            break;
        }    

        case 4:
        {
            gboolean error_detected = FALSE;
            
            calibrated_tokens_ = split(output_data, ',');
            serial_port_->unsetWriteFunc(std::bind(&AS7265xUnit::dataReply, this, std::placeholders::_1));
            sequence_no_= 0;

            //Organise and write out the channel data in order
            for (gint i = 0; i < order_.size(); ++i) {
                gint index = order_[i];
                temp_data << channels_[index] << "," << raw_tokens_[index] << "," << calibrated_tokens_[index];
                if ((output_file_->writeLineToFile(temp_data.str(), &error)) == -1){
                    error_detected = TRUE;
                    break;
                }
                temp_data.str(""); //Clear the temp buffer for re-use
            }

            raw_tokens_.clear();
            calibrated_tokens_.clear();

            if (!error_detected){//Clear buffers for next time
               if ((output_file_->writeLineToFile(temp_data.str(), &error)) == -1) //Add blank line below data readout
                goto error;
            }
            else
                goto error;

            break;

            /*Channel order: 9, 11, 13, 14, 15, 16, 7, 8, 10, 12, 17, 18, 1, 2, 3, 4, 5, 6
            * Channel order indexes: 8, 10, 12, 13, 14, 15, 6, 7, 9, 11, 16, 17, 0, 1, 2, 3, 4, 5
            * Channel labels: 610, 680, 730, 760, 810, 860, 560, 585, 645, 705, 900, 940, 410, 435, 460, 485, 510, 535*/
        }
    }

    return;

    error:
    sequence_no_= 9; //I'll use this number to deal with an error
    serial_port_->unsetWriteFunc(std::bind(&AS7265xUnit::dataReply, this, std::placeholders::_1));
    runData(&error);
}