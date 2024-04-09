#include <algorithm> // for std::max_element and std::min_element
#include <cmath>

#include "cdaf.h"
#include "AdditionsForAF.h"
#include "ErrorHandler.h"

CDAF::CDAF(AF_Additions* AF_interface, ErrorHandler* error_handler) : 
    my_AF_interface_(AF_interface),
    error_handler_(error_handler),
    i2c_focus_controller_("camera-0"),
    currentState_(new TransitState()),
    transitTo(MAX_FOCUS_INDEX),
    transitToDetail(FALSE),
    focusIndex(280), chaseFocus(0), movingFocusIn(TRUE) {

    g_print ("...autofocus CDAF control algorithm\n"); 
}

//CDAF::~CDAF() = default;
CDAF::~CDAF(){
    if(currentState_) 
            delete currentState_;
     g_print("CDAF autofocus algorithm shutdown...\n");
}

gint CDAF::setup(GError** error){
    return i2c_focus_controller_.setup(error);
}

//These can be modified to pass the error pointer through
void CDAF::runFocus(gfloat focus_value) {
    GError* error = nullptr;
    focusValue = focus_value; //Give the focus machine the latest focus value to work with
    currentState_->runFocus(*this);
    if ((i2c_focus_controller_.setFocus(focusIndex, &error)) == -1)
        error_handler_->errorHandler(&error);
}

//Pass the error pointer through
gint CDAF::setFocus(guint focus_index, GError** error) {
    return i2c_focus_controller_.setFocus(focus_index, error);
}

void CDAF::changeState(FocusState* newState) {
        delete currentState_;
        currentState_ = newState;
        g_print("AF CHANGED STATE\n");
}

void CDAF::focusAchieved(){
    my_AF_interface_->focusAchieved();
}

void CDAF::setScanning(gboolean value, guint timeout){
    my_AF_interface_->setScanning(value, timeout);
}

void TransitState::runFocus(CDAF & cdaf) {
    //This sets the parameters to start scanning in
    gint travelRemaining = cdaf.transitTo - cdaf.focusIndex;
    cdaf.setScanning(FALSE, 250);

    if (std::abs(travelRemaining) > TRANSIT_STEP){
        g_print("TRAVEL REMAINAING :%d\n", travelRemaining);
        if (travelRemaining > 0)
            cdaf.focusIndex = cdaf.focusIndex + TRANSIT_STEP;
        else
            cdaf.focusIndex = cdaf.focusIndex - TRANSIT_STEP;
    }
    else{
        cdaf.focusIndex = cdaf.transitTo;

        if(cdaf.transitToDetail) {
            /*CHANGE STATE HERE StartDetailScanState*/
            g_print("CHANGING STATE TO: StartDetailScan\n");
            cdaf.changeState(new StartDetailScanState());
        }
        else {  
            /*CHANGE STATE HERE StartScanFocusInState*/
            g_print("CHANGING STATE TO: StartScanFocusIn\n");
            cdaf.changeState(new StartScanFocusInState());
        }  
    }  
}

void StartScanFocusInState::runFocus(CDAF & cdaf) {
    //This sets the parameters to start scanning in
    cdaf.scanInValues.clear();
    cdaf.scanInIndicies.clear();
    cdaf.focusStep = 10;
    cdaf.focusIndex = MAX_FOCUS_INDEX;
    cdaf.boundary = FALSE;
    cdaf.setScanning(TRUE, 100);
    
    /*CHANGE STATE HERE ScanFocusInState*/
    g_print("CHANGING STATE TO: ScanFocusIn\n");
    cdaf.changeState(new ScanFocusInState());
    
}

void ScanFocusInState::runFocus(CDAF & cdaf) {
    
    cdaf.scanInValues.push_back(cdaf.focusValue);
    cdaf.scanInIndicies.push_back(cdaf.focusIndex);

    if (!cdaf.boundary){
        cdaf.focusIndex = (cdaf.focusIndex - cdaf.focusStep);
    }
    else {
        /*CHANGE STATE HERE StartScanFocusOutState */
        g_print("CHANGING STATE TO: StartScanFocusOut\n");
        cdaf.changeState(new StartScanFocusOutState());
        cdaf.focusIndex = MIN_FOCUS_INDEX;
    }

    if (cdaf.focusIndex <= MIN_FOCUS_INDEX){
        cdaf.boundary = TRUE; 
        cdaf.focusIndex = MIN_FOCUS_INDEX;
    }
}

void StartScanFocusOutState::runFocus(CDAF & cdaf) {
     //This sets the parameters to start scanning in
    cdaf.scanOutValues.clear();
    cdaf.scanOutIndicies.clear();
    cdaf.focusStep = 10;
    cdaf.focusIndex = MIN_FOCUS_INDEX;
    cdaf.boundary = FALSE;
    cdaf.setScanning(TRUE, 100);

    /*CHANGE STATE HERE ScanFocusOutState*/
    g_print("CHANGING STATE TO: ScanFocusOut\n");
    cdaf.changeState(new ScanFocusOutState());
}

void ScanFocusOutState::runFocus(CDAF & cdaf) {
    
    cdaf.scanOutValues.push_back(cdaf.focusValue);
    cdaf.scanOutIndicies.push_back(cdaf.focusIndex);

    if (!cdaf.boundary){
        cdaf.focusIndex = (cdaf.focusIndex + cdaf.focusStep);
    }
    else {
        auto max_it = std::max_element(cdaf.scanInValues.begin(), cdaf.scanInValues.end());
        guint scanInMax = std::distance(cdaf.scanInValues.begin(), max_it);
        auto max_it1 = std::max_element(cdaf.scanOutValues.begin(), cdaf.scanOutValues.end());
        guint scanOutMax = std::distance(cdaf.scanOutValues.begin(), max_it1);

        cdaf.detailScanMax = cdaf.scanInIndicies[scanInMax] + 10;
        cdaf.detailScanMin = cdaf.scanOutIndicies[scanOutMax] - 10;

        cdaf.transitTo = cdaf.detailScanMax;
        cdaf.transitToDetail = TRUE;
        /*CHANGE STATE HERE TransitState */
        g_print("CHANGING STATE TO: Transit\n");
        cdaf.changeState(new TransitState());
        cdaf.focusIndex = MAX_FOCUS_INDEX;
    }

    if (cdaf.focusIndex >= MAX_FOCUS_INDEX){
        cdaf.boundary = TRUE; 
        cdaf.focusIndex = MAX_FOCUS_INDEX;
    }
}

void StartDetailScanState::runFocus(CDAF & cdaf) {
    
    cdaf.scanInValues.clear();
    cdaf.scanInIndicies.clear();
    cdaf.scanOutValues.clear();
    cdaf.scanOutIndicies.clear();
    cdaf.focusStep = 2;
    cdaf.focusIndex = cdaf.detailScanMax;
    cdaf.boundary = FALSE;
    cdaf.setScanning(TRUE, 150);

    /*CHANGE STATE HERE DetailScanState*/
    g_print("CHANGING STATE TO: DetailScan\n");
    cdaf.changeState(new DetailScanState());

    //g_print("END SCAN FOCUS SET\n");
}

void DetailScanState::runFocus(CDAF & cdaf) {
    
    cdaf.scanInValues.push_back(cdaf.focusValue);
    cdaf.scanInIndicies.push_back(cdaf.focusIndex);

    if (!cdaf.boundary){
        cdaf.focusIndex = (cdaf.focusIndex - cdaf.focusStep);
    }
    else {
        //now we have a focus value
        /*CHANGE STATE HERE SetFocusState*/
        g_print("CHANGING STATE TO: SetFocus\n");
        cdaf.changeState(new SetFocusState());
        cdaf.focusIndex = cdaf.detailScanMin;
    }

    if (cdaf.focusIndex <= cdaf.detailScanMin){
        cdaf.boundary = TRUE; 
        cdaf.focusIndex = MIN_FOCUS_INDEX;
    }     
}

void SetFocusState::runFocus(CDAF & cdaf) {

    auto max_it = std::max_element(cdaf.scanInValues.begin(), cdaf.scanInValues.end());
    guint index = std::distance(cdaf.scanInValues.begin(), max_it);

    g_print("INDEX: %d (0 - %d)\n", index, (int)(cdaf.scanInValues.size()-1));

    //if the maximum was the first or last element of the vector array
    if ((index == 0) || (index == (cdaf.scanInValues.size()-1))){
        cdaf.chaseFocus++;

        if (index == 0){
            cdaf.detailScanMax = cdaf.scanInIndicies[index] + 40;
            cdaf.detailScanMin = cdaf.scanInIndicies[index];
        }
        else {
            cdaf.detailScanMax = cdaf.scanInIndicies[index];
            cdaf.detailScanMin = cdaf.scanInIndicies[index] - 40;
        }

    }
    else{
        cdaf.chaseFocus = 0;
        cdaf.detailScanMax = cdaf.scanInIndicies[index] + 20;
        cdaf.detailScanMin = cdaf.scanInIndicies[index] - 20;
    } 

    if (cdaf.detailScanMax > MAX_FOCUS_INDEX)
        cdaf.detailScanMax = MAX_FOCUS_INDEX;
    if (cdaf.detailScanMin < MIN_FOCUS_INDEX)
        cdaf.detailScanMin = MIN_FOCUS_INDEX;
   
    cdaf.focusIndex = cdaf.scanInIndicies[index];

    if (cdaf.chaseFocus == 0){
        cdaf.setScanning(TRUE, 300); //Nice long timeout here
        /*CHANGE STATE HERE GrabFocusValueState*/
        g_print("CHANGING STATE TO: GrabFocusValue\n");
        cdaf.changeState(new GrabFocusValueState());
    }
    else if (cdaf.chaseFocus > 2){
        cdaf.chaseFocus = 0;
        cdaf.transitToDetail = FALSE;
        cdaf.transitTo = MAX_FOCUS_INDEX;
        /*CHANGE STATE HERE TransitState*/
        g_print("CHANGING STATE TO: Transit\n");
        cdaf.changeState(new TransitState());
    }
    else {
        /*CHANGE STATE HERE StartDetailScanState*/
        g_print("CHANGING STATE TO: StartDetailScan\n");
        cdaf.changeState(new StartDetailScanState());
    }    
          
}

void GrabFocusValueState::runFocus(CDAF & cdaf) {
    //Don't change the index, just grab the value
    g_print ("cdaf.focusValue: %f", cdaf.focusValue);
    cdaf.focusAchieved();
    cdaf.setScanning(FALSE, 250);
    /*CHANGE STATE HERE StartDriftScanningState*/
    g_print("CHANGING STATE TO: StartDriftScanning\n");
    cdaf.changeState(new StartDriftScanningState());
}

void StartDriftScanningState::runFocus(CDAF & cdaf) {
    cdaf.scanInValues.clear();
    cdaf.scanInIndicies.clear();
    cdaf.scanOutValues.clear();
    cdaf.scanOutIndicies.clear();
    cdaf.focusStep = 5;
    cdaf.boundary = FALSE;
    cdaf.movingFocusIn = TRUE;
    cdaf.setScanning(TRUE, 150);

    /*CHANGE STATE HERE ConfirmDriftDirectionState*/
    g_print("CHANGING STATE TO: ConfirmDriftDirection\n");
    cdaf.changeState(new ConfirmDriftDirectionState());

}

void ConfirmDriftDirectionState::runFocus(CDAF & cdaf) {
    //If direction is worsening, then throw away array start
    //fresh in sacn for peak. If improving keep until a peak is found
    cdaf.scanInValues.push_back(cdaf.focusValue);
    cdaf.scanInIndicies.push_back(cdaf.focusIndex);

    auto max_it = std::max_element(cdaf.scanInValues.begin(), cdaf.scanInValues.end());
    gboolean max_at_start = ((std::distance(cdaf.scanInValues.begin(), max_it)) <= 2);
    //gboolean max_at_end = ((std::distance(max_it, cdaf.scanInValues.end())) == cdaf.scanInValues.size() -1);
    guint index = std::distance(max_it, cdaf.scanInValues.end());

    if (cdaf.scanInValues.size() >= 5 ){
        if (max_at_start){ //we are going in the wrong direction
            cdaf.movingFocusIn = FALSE;
            cdaf.scanInValues.clear();
            cdaf.scanInIndicies.clear();
        }
        /*CHANGE STATE HERE DriftScanForPeakState*/
        g_print("CHANGING STATE TO: DriftScanForPeak\n");
        cdaf.changeState(new DriftScanForPeakState());
    }

    cdaf.focusIndex = cdaf.focusIndex - cdaf.focusStep;
    if (cdaf.focusIndex < MIN_FOCUS_INDEX){
        cdaf.focusIndex = MIN_FOCUS_INDEX;
        cdaf.transitToDetail = FALSE; //Now we do a full scan
        cdaf.transitTo = MAX_FOCUS_INDEX;
        /*CHANGE STATE HERE TransitState*/
        g_print("CHANGING STATE TO: Transit\n");
        cdaf.changeState(new TransitState());
    }
}

void DriftScanForPeakState::runFocus(CDAF & cdaf) {
    
    cdaf.scanInValues.push_back(cdaf.focusValue);
    cdaf.scanInIndicies.push_back(cdaf.focusIndex);

    auto max_it = std::max_element(cdaf.scanInValues.begin(), cdaf.scanInValues.end());
    guint index = std::distance(max_it, cdaf.scanInValues.end());

    if (index < 5){ //then we have not gone past a peak yet
         //give up at some point
        if (cdaf.movingFocusIn){
            cdaf.focusIndex = cdaf.focusIndex - cdaf.focusStep;
            if (cdaf.focusIndex < MIN_FOCUS_INDEX){
                cdaf.boundary = TRUE;
                cdaf.focusIndex = MIN_FOCUS_INDEX;
            }
        }
        else {
            cdaf.focusIndex = cdaf.focusIndex + cdaf.focusStep;
            if (cdaf.focusIndex > MAX_FOCUS_INDEX){
                cdaf.boundary = TRUE;
                cdaf.focusIndex = MAX_FOCUS_INDEX;
            }
        }
        if ((cdaf.scanInValues.size() > 50) || (cdaf.boundary)){ //These are the give up conditions
            cdaf.transitToDetail = FALSE; //Now we do a full scan
            cdaf.transitTo = MAX_FOCUS_INDEX;
            /*CHANGE STATE HERE TransitState*/
            g_print("CHANGING STATE TO: Transit\n");
            cdaf.changeState(new TransitState());
        }
    }
    else{ //we have a peak lets call it focussed
        index = std::distance(cdaf.scanInValues.begin(), max_it); 
        cdaf.focusIndex = cdaf.scanInIndicies[index];
        cdaf.setScanning(TRUE, 300); //Nice long timeout here 
        /*CHANGE STATE HERE GrabFocusValueState*/
        g_print("CHANGING STATE TO: GrabFocusValue\n");
        cdaf.changeState(new GrabFocusValueState());
    }

}
