#include "AdditionsForAF.h"
#include "AdditionsParent.h"
#include "opencv4/opencv2/imgproc.hpp"
//#include "opencv4/opencv2/highgui.hpp"

AF_Additions::AF_Additions(AdditionsParent* additions_parent,ErrorHandler* error_handler):
grab_focus_frame_(FALSE),focussed_(FALSE), focussing_(FALSE),
focus_lock_(FALSE), focus_value_(0),focussed_value_(0), focus_frame_timeout_(250),
focus_machine_(this, error_handler),
additions_parent_(additions_parent) {
        //stepInDirection = FALSE; //This in the algorithm class constructor
    g_print("...AF addional objects created\n");
}

AF_Additions::~AF_Additions(){
    g_print("AF addional objects removed...\n");
}

gint AF_Additions::setup(GError** error) {

    if ((focus_machine_.setup(error)) == -1)
        return -1; //error should be set

    //Change this so setFocus does error return as well, then we can fail if
    //setFocus returns a fail. Same as above!
    if ((focus_machine_.setFocus(280, error)) == -1) //We set a focus value here
        return -1; //Assume error is already set

    triggerFocusCapture(); //Grab a frame to start
    g_idle_add(focusTriggerWrapper, this);

    g_print ("AF additions focus controller setup\n"); 
    return 0;
}

void AF_Additions::focusAchieved() {
    focussed_value_= focus_value_;
    g_print("Focussed_Value_ is %f\n", focussed_value_);
    focussed_ = TRUE;
}

gboolean AF_Additions::setFocusLock(){
    focus_lock_ = TRUE;
}

void AF_Additions::setScanning(gboolean value, guint timeout){
    scanning_ =  value;
    focus_frame_timeout_ = timeout;
 }

gboolean AF_Additions::releaseFocusLockWrapper(gpointer user_data) {
    return reinterpret_cast<AF_Additions*>(user_data)->releaseFocusLock(user_data);
}

gboolean AF_Additions::releaseFocusLock(gpointer user_data)
{
    AF_Additions* self = static_cast<AF_Additions*>(user_data);
    /*Need to release the focus lock after capturing image*/
    g_print("Releasing Focus Lock\n");
    self->focus_lock_ = FALSE;
    self->focus_value_ = 0;
    self->focussing_ = FALSE;
    if (self->focussed_)
        g_timeout_add(250, focusTriggerWrapper, this);
    else
        g_idle_add(focusTriggerWrapper, this);

    return FALSE;
}

/**
  * Send buffer to focus engine. This is a callback function!
  * Callback set in create_focus_enc_bin
  *
  * @param fsink  : image sink
  * @param buffer : gst buffer
  * @param pad    : element pad
  * @param udata  : the gpointer to user data
  */
/*gboolean AF_Additions::focusImageCapturedWrapper(GstElement* fsink,
    GstBuffer* buffer, GstPad* pad, gpointer user_data){
        return reinterpret_cast<AF_Additions*>(user_data)->focusImageCaptured(fsink,
        buffer, pad, user_data);
}*/

gboolean AF_Additions::focusImageCaptured(GstElement* fsink,
    GstBuffer* buffer, GstPad* pad, gpointer user_data)
{
    AF_Additions* self = static_cast<AF_Additions*>(user_data);
    GstMapInfo info;
    gfloat difference;

    //g_print("FOCUS IMAGE CAPTURE CALLED\n");

    self->additions_parent_->closeFocusValve();

    //if (self->grab_focus_frame_) { //and perhaps && app->focusValue == 0 or NULL
    if (gst_buffer_map(buffer, &info, GST_MAP_READ)) {
        if (info.size) {
            //self->grab_focus_frame_ = FALSE;
            
            self->focus_value_= laplacianMean(&info);        }

        gst_buffer_unmap(buffer, &info);

        if (self->focussed_) {

            difference = (self->focussed_value_ - self->focus_value_);
            if (difference < 0)
                difference = (self->focus_value_ - self->focussed_value_);

            g_print("Focus Value: %f\n", self->focus_value_);
            g_print("Last Focus Value: %f\n", self->focussed_value_);
            g_print("Difference check: %f\n", difference);

            //Greater than a 10% difference
            if (difference >= (0.1 * self->focussed_value_)) {
                
                //g_print("Current Focus Value: %d\n", app->focusValue);
                //g_print("Best Focus Value: %d\n", app->bestFocusValue);
                //g_print("Focus Index: %d\n", app->lastSetFocusIndex);
                self->focussed_ = FALSE;
                self->focussing_ = TRUE;
            }
        }

        //Here we run the focussing state machine when idle
        if (!self->focussed_) {
            g_idle_add(self->runFocusWrapper, self);
        }
        else { //Here we grab another frame to check against our focussed value
            self->focussing_ = FALSE;
            self->focus_value_ = 0;
            g_timeout_add(250, self->focusTriggerWrapper, self);
        }

    }
       /* else {
            //Error Handling in HERE!
            //NVGST_WARNING_MESSAGE("focus buffer probe failed\n");
        } 
    }*/
    return TRUE;
}

/*A C++ library to access opencv from nvgstcapture-1*/
/****************************************************
 * This is what I'm trying to replicate
 * 
 *   def sobel(img):
 *       img_gray = cv2.cvtColor(img,cv2.COLOR_RGB2GRAY)
 *       img_sobel = cv2.Sobel(img_gray,cv2.CV_16U,1,1)
 *       return cv2.mean(img_sobel)[0]
 *
 *   def laplacian(img):
 *       img_gray = cv2.cvtColor(img,cv2.COLOR_RGB2GRAY)
 *       img_sobel = cv2.Laplacian(img_gray,cv2.CV_16U)
 *       return cv2.mean(img_sobel)[0]
 *****************************************************/

gfloat AF_Additions::laplacianMean (GstMapInfo *padinfo)
{
    gint resolution_width, resolution_height,x, y;

    additions_parent_->getResolution(&resolution_width, &resolution_height);

    x = resolution_width/2;
    y = resolution_height/2;

    cv::Mat frame(cv::Size(resolution_width, resolution_height), CV_8UC1, (char *)padinfo->data, cv::Mat::AUTO_STEP);
    //cv::Mat frame = cv::Mat((height * 3/2),width, CV_8UC1, (guchar *)padinfo->data, cv::Mat::AUTO_STEP);
    cv::Mat cropped = frame(cv::Range(y-100,y+100), cv::Range(x-100, x+100));
    //cv::Mat bgrFrame;
    //cv::Mat grayFrame;
    cv::Mat laplacianFrame;

    //cv::cvtColor(cropped,bgrFrame, cv::COLOR_RGBA2BGR);
    //cv::cvtColor(bgrFrame, grayFrame, cv::COLOR_BGR2GRAY);
    cv::Laplacian(cropped, laplacianFrame, CV_16U);

    //cv::imshow("Gray Frame", bgrFrame);

    return (gfloat)cv::mean(laplacianFrame)[0];
}

void AF_Additions::triggerFocusCapture(void)
{
    additions_parent_->openFocusValve();
    //grab_focus_frame_ = TRUE;
    focus_value_ = 0;
    focussing_ = TRUE;
}

gboolean AF_Additions::focusTriggerWrapper(gpointer user_data) {
    return reinterpret_cast<AF_Additions*>(user_data)->focusTrigger(user_data);
}
gboolean AF_Additions::focusTrigger(gpointer user_data)
{
    AF_Additions* self = static_cast<AF_Additions*>(user_data);
    //g_print("Hitting idleFocus_trigger\n");
    if ((!self->focus_lock_) && (!self->focussing_) && (self->focus_value_ == 0)) {
        //g_print("Hitting the trigger\n");
        self->triggerFocusCapture();
        return FALSE;
    }
    return TRUE;
}

gboolean AF_Additions::runFocusWrapper(gpointer user_data) {
    return reinterpret_cast<AF_Additions*>(user_data)->runFocus(user_data);
}

gboolean AF_Additions::runFocus(gpointer user_data)
{
    AF_Additions* self = static_cast<AF_Additions*>(user_data);

    //Need to set this to run the focus algorithm
    //g_print ("Running focus with value: %d\n", self->AF_Additions::focus_value_);
    self->focus_machine_.runFocus(self->AF_Additions::focus_value_);
    self->focus_value_ = 0;
    self->focussing_ = FALSE;

    if ((self->focussed_) || (self->scanning_))
        g_timeout_add(focus_frame_timeout_, self->focusTriggerWrapper, self);
    else
        g_idle_add(self->focusTriggerWrapper, self);

    return FALSE;
}
