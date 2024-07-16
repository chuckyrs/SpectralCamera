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