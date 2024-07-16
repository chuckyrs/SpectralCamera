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

#include "AdditionsForAF.h"
#include "AdditionsParent.h"
#include "opencv4/opencv2/imgproc.hpp"

/**
 * Constructs an AF_Additions object associated with a specific camera using its identifier.
 * 
 * @param * additionsParent : Pointer to the application's additions parent object
 * @param * error_handler : Pointer to the application's error_handler object
 * 
 */
AF_Additions::AF_Additions(AdditionsParent* additions_parent,ErrorHandler* error_handler):
grab_focus_frame_(FALSE),focussed_(FALSE), focussing_(FALSE),
focus_lock_(FALSE), focus_value_(0),focussed_value_(0), focus_frame_timeout_(250),
focus_machine_(this, error_handler),
additions_parent_(additions_parent) {
    g_print("...AF addional objects created\n");
}

/**
 * Destructor for AF_Additions. Logs the shutdown process and cleans up resources.
 */
AF_Additions::~AF_Additions(){
    g_print("AF addional objects removed...\n");
}

/**
 * Setup for AF_Additions. Part of the heirachial setup chain that occurs
 * only after the camera has come online.
 * 
 * @param error : Pointer to the nvgstcapture-1.0 error struct for error reporting
 *
 * @return : -1 on error, otherwise 0.
 */
gint AF_Additions::setup(GError** error) {

    if ((focus_machine_.setup(error)) == -1)
        return -1; //error should be set

    if ((focus_machine_.setFocus(280, error)) == -1) //We set a focus value here
        return -1; //Assume error is already set

    triggerFocusCapture(); //Grab a frame to start
    g_idle_add(focusTriggerWrapper, this);

    g_print ("AF additions focus controller setup\n"); 
    return 0;
}

/**
* A public function so that the runFocus algorithm can notify us that focus is set.
*/
void AF_Additions::focusAchieved() {
    focussed_value_= focus_value_;
    focussed_ = TRUE;
}

/**
* A public function so that the sysCtrl can notify us that the button pressed and we need to
* hold the current focus.
*/
gboolean AF_Additions::setFocusLock(){
    focus_lock_ = TRUE;
}

/**
* A public function so that the runFocus algorithm can notify us that it is scanning for focus,
* and at what interval it would like focus frames.
*/
void AF_Additions::setScanning(gboolean value, guint timeout){
    scanning_ =  value;
    focus_frame_timeout_ = timeout;
 }

/**
* CALLBACK FUNCTION. Release the focus lock after a g_timeout.This wrapper reinterprets the
* gpointer user_data object into usable pointer for accessing the setup method in the AdditionsParent class.
*
* @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
*/

gboolean AF_Additions::releaseFocusLockWrapper(gpointer user_data) {
    return reinterpret_cast<AF_Additions*>(user_data)->releaseFocusLock(user_data);
}

/**
 * CLASS METHOD. Release the focus lock after a g_timeout. We static cast the user_data pointer to access its methods.
 *
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AdditionsParent object
 *
 * @return : return FALSE will ensure the timeout does not run again.
 */
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
 * CALLBACK FUNCTION. Send buffer to focus engine. There is no wrapper function here as this is called from
 * nvgstcapture-1.0 through the focusImageCaptured_C() function.
 * 
 * @param fsink  : image sink
 * @param buffer : gst buffer
 * @param pad    : element pad
 * @param udata  : the gpointer to user data
 * 
 * @return TRUE : Keeps the callback alive
 */

gboolean AF_Additions::focusImageCaptured(GstElement* fsink,
    GstBuffer* buffer, GstPad* pad, gpointer user_data)
{
    AF_Additions* self = static_cast<AF_Additions*>(user_data);
    GstMapInfo info;
    gfloat difference;

    self->additions_parent_->closeFocusValve();

    if (gst_buffer_map(buffer, &info, GST_MAP_READ)) {
        if (info.size) {            
            self->focus_value_= laplacianMean(&info);        }

        gst_buffer_unmap(buffer, &info);

        if (self->focussed_) {

            difference = (self->focussed_value_ - self->focus_value_);
            if (difference < 0)
                difference = (self->focus_value_ - self->focussed_value_);

            //Greater than a 10% difference
            if (difference >= (0.1 * self->focussed_value_)) {
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
    return TRUE;
}


/****************************************************
 * This is what I'm trying to replicate - From Arducam
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


/**
* This  is center of the autofocus system. This takes an image frame and uses a
* laPlacian function to calculate a focus value.
* 
* @param padinfo : This the frame buffer from the Gstreamer stream.
* 
* @return : The focus value as a floating point number.
*/
gfloat AF_Additions::laplacianMean (GstMapInfo *padinfo)
{
    gint resolution_width, resolution_height,x, y;

    additions_parent_->getResolution(&resolution_width, &resolution_height);

    x = resolution_width/2;
    y = resolution_height/2;

    cv::Mat frame(cv::Size(resolution_width, resolution_height), CV_8UC1, (char *)padinfo->data, cv::Mat::AUTO_STEP);
    cv::Mat cropped = frame(cv::Range(y-100,y+100), cv::Range(x-100, x+100));
    cv::Mat laplacianFrame;
    cv::Laplacian(cropped, laplacianFrame, CV_16U);

    return (gfloat)cv::mean(laplacianFrame)[0];
}

/**
* By opening the focus valve, we let frames onto the GStreamer focus stream.
*/
void AF_Additions::triggerFocusCapture(void)
{
    additions_parent_->openFocusValve();
    focus_value_ = 0;
    focussing_ = TRUE;
}

/**
 * CALLBACK FUNCTION. Trigger the capture of a focus frame after a g_timeout. This wrapper reinterprets the
 * gpointer user_data object into usable pointer for accessing methods in the AF_Additions class.
 *
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AF_Additions object
 */
gboolean AF_Additions::focusTriggerWrapper(gpointer user_data) {
    return reinterpret_cast<AF_Additions*>(user_data)->focusTrigger(user_data);
}

/**
 * CLASS METHOD. Trigger the capture of a focus frame after a g_timeout. We static cast the user_data pointer to access its methods.
 *
 * @param user_data : Standard glib function parameter, used to pass a pointer to this AF_Additions object
 *
 * @return : return FALSE if we are ready to capture a frame and finish, otherwise wait and repeat later.
 */
gboolean AF_Additions::focusTrigger(gpointer user_data)
{
    AF_Additions* self = static_cast<AF_Additions*>(user_data);
    if ((!self->focus_lock_) && (!self->focussing_) && (self->focus_value_ == 0)) {
        self->triggerFocusCapture();
        return FALSE;
    }
    return TRUE;
}

/**
* CALLBACK FUNCTION. Run the AF runFocus state machine in the AF interface after an AF frame has been captured.
* This wrapper reinterprets the * gpointer user_data object into usable pointer for accessing methods
* in the AF_Additions class.
*
* @param user_data : Standard glib function parameter, used to pass a pointer to this AF_Additions object
*/
gboolean AF_Additions::runFocusWrapper(gpointer user_data) {
    return reinterpret_cast<AF_Additions*>(user_data)->runFocus(user_data);
}

/**
* CLASS METHOD. Run the AF runFocus state machine in the AF interface after an AF frame has been captured.
* We static cast the user_data pointer to access its methods.
*
* @param user_data : Standard glib function parameter, used to pass a pointer to this AF_Additions object
*
* @return : return FALSE will ensure the timeout does not run again.
*/
gboolean AF_Additions::runFocus(gpointer user_data)
{
    AF_Additions* self = static_cast<AF_Additions*>(user_data);

    //Need to set this to run the focus algorithm
    self->focus_machine_.runFocus(self->AF_Additions::focus_value_);
    self->focus_value_ = 0;
    self->focussing_ = FALSE;

    if ((self->focussed_) || (self->scanning_))
        g_timeout_add(focus_frame_timeout_, self->focusTriggerWrapper, self);
    else
        g_idle_add(self->focusTriggerWrapper, self);

    return FALSE;
}
