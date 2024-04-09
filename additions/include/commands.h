#ifndef COMMANDS_H
#define COMMANDS_H

#define AT_ACK "AT"
#define LEN_ACK_COMMAND 2
#define AT_HARDWARE_VERSION "ATVERHW"
#define LEN_HARDWARE_VERSION_COMMAND 7
#define AT_SOFTWARE_VERSION "ATVERSW"
#define LEN_SOFTWARE_VERSION_COMMAND 7
#define AT_SENSORS_PRESENT "ATPRES"
#define LEN_SENSORS_PRESENT_COMMAND 6
#define AT_MEASURE_MODE "ATTCSMD"
#define LEN_MEASURE_MODE_COMMAND 7
#define AT_INTEGRATION_TIME "ATINTTIME"
#define LEN_INTEGRATION_TIME_COMMAND 9
#define AT_GAIN "ATGAIN"
#define LEN_GAIN_COMMAND 6
#define AT_SET_INTEGRATION_TIME "ATINTTIME=255"
#define LEN_SET_INTEGRATION_TIME_COMMAND 13
#define AT_SET_GAIN "ATGAIN=0"
#define LEN_SET_GAIN_COMMAND 8
#define AT_DATA "ATDATA"
#define LEN_DATA_COMMAND 6
#define AT_CALIBRATED_DATA "ATCDATA"
#define LEN_CALIBRATED_DATA_COMMAND 7
#define AT_SENSOR_TEMP "ATTEMP"
#define LEN_SENSOR_TEMP_COMMAND 6

#endif //COMMANDS_H