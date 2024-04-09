#include "amsAS7265x.h"
#include "commands.h"
#include "ErrorHandler.h"


AS7265xUnit::AS7265xUnit(SerialPort* serial_port, OutputFileControl* file_to_write, ErrorHandler* error_handler) :
    sequence_no_(0), 
    serial_port_(serial_port), 
    output_file_(file_to_write),
    error_handler_(error_handler)  {
    
    g_print ("...AS7265x communications controller\n");
}

AS7265xUnit::~AS7265xUnit(){
    g_print("Shutting down AS7265x communications controller\n");
    g_print("Nothing to do here\n");
};

//This is a void setup becuase of no way to get complete handshake before function return.
//Any error will result in shutdown anyway
void AS7265xUnit::getHandshakeData() {
    GError* error = nullptr;

    g_print("Setting up AS7265x communications controller\n");
    g_print("Running handshake with AS7265x device...\n");

    runHandshake(&error);
    //If there is an error in the handshake we'll let the error handler pick it up
}

gboolean AS7265xUnit::getAS7265xDataWrapper(gpointer user_data){
    return reinterpret_cast<AS7265xUnit*>(user_data)->getAS7265xData();
}

gboolean AS7265xUnit::getAS7265xData(void)
{
    GError* error = nullptr;
    runData(&error);

    return FALSE;
}

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
    g_print("%s... ", command.c_str());

    return;

    error:

    error_handler_->errorHandler(error);

}

void AS7265xUnit::runData(GError** error)
{
    //g_print("\nData acquisition running...\n");
    switch (sequence_no_)
    {
        case 0:
            serial_port_->setWriteFunc(std::bind(&AS7265xUnit::dataReply, this, std::placeholders::_1));

            //serial_port_->setWriteFunc(dataReply);
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

// Static method
/*void AS7265xUnit::handshakeReplyStatic(const std::string& output_data) {
    return static_cast<AS7265xUnit*>-()>handshakeReply(output_data);
}*/

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
            //output_file_->writeLineToFile(temp_data.str(), &error);
            sequence_no_++;
            runHandshake(&error);
            break;

        case 2:
            temp_data << "AS7265x Sofware Version," << output_data.substr(0, output_data.size());
            if (output_file_->writeLineToFile(temp_data.str(), &error) == -1)
                goto error;
            //output_file_->writeLineToFile(temp_data.str(), &error);
            sequence_no_++;
            runHandshake(&error);
            break;

        case 3:      
            temp_data << "Sensors working," << output_data.substr(0, output_data.size() );
            if (output_file_->writeLineToFile(temp_data.str(), &error) == -1)
                goto error;
            //output_file_->writeLineToFile(temp_data.str(), &error);
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