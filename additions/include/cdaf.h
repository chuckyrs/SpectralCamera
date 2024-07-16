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

#ifndef CDAF_H
#define CDAF_H


#define MAX_FOCUS_INDEX 900
#define MIN_FOCUS_INDEX 50
#define TRANSIT_STEP    10

#include <glib.h>
#include <vector>
#include "I2CsetFocus.h"

/*When you create a pointer to a class, you can get away with the class AF_Additions declaration, and put
* #include "AdditionsForAF.h" in the source file.
*When you are storing the actual class object, you need to have #include "I2CsetFocus.h" in the header*/

class ErrorHandler;
class AF_Additions;
class FocusState;
class StartScanFocusInState;
class ScanFocusInState;
class StartScanFocusOutState;
class ScanFocusOutState;
class StartDetailScanState;
class DetailScanState;
class SetFocusState;
class GrabFocusValueState;
class StartDriftScanningState;
class ConfirmDriftDirectionState;
class DriftScanForPeakState;

/*This basically is the algorithm in its entirity. This will need to have a  pointer to its parent  object for get and set 
functions. This is also where the I2CsetFocus object will need to be */
class CDAF {
public:
    CDAF(AF_Additions* AF_interface, ErrorHandler* error_handler);
    ~CDAF();
    gint setup(GError** error);
    void runFocus(gfloat focus_value);
    gint setFocus(guint focus_index, GError** error);
    void changeState(FocusState* newState);
    void focusAchieved();
    void setScanning(gboolean value, guint timeout);

    std::vector<float> scanInValues;
    std::vector<int> scanInIndicies;

    std::vector<float> scanOutValues;
    std::vector<int> scanOutIndicies;

    gfloat focusValue;
    guint focusIndex;
    guint focusStep;
    gboolean boundary;
    gboolean transitToDetail;
    gboolean movingFocusIn;
    gint transitTo;
    guint detailScanMax;
    guint detailScanMin;
    guint chaseFocus;

private:
    AF_Additions* my_AF_interface_;
    ErrorHandler* error_handler_;
    FocusState* currentState_;
    CameraI2CDevice i2c_focus_controller_;
    FocusState* postTransitState_;
    
    
    //needs a pointer to it's parent.
};


//each runfocus will need to return the next setFocus value.

class FocusState {
public:
    virtual ~FocusState() = default;
    virtual void runFocus(CDAF& cdaf) = 0;
};

class TransitState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

class StartScanFocusInState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

class ScanFocusInState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

class StartScanFocusOutState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

class ScanFocusOutState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

class StartDetailScanState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

class DetailScanState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

class SetFocusState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

class GrabFocusValueState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

class StartDriftScanningState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

class ConfirmDriftDirectionState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

class DriftScanForPeakState : public FocusState {
public:
    void runFocus(CDAF& cdaf) override;
};

#endif //CDAF_H