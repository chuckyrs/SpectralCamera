#ifndef ADDITIONSFORAF_H
#define ADDITIONSFORAF_H

#include <gst/gst.h>
#include <glib.h>

#include "cdaf.h"

class ErrorHandler;
class AdditionsParent;

class AF_Additions {
public:
    AF_Additions(AdditionsParent* additions_parent, ErrorHandler* error_handler);
    ~AF_Additions();
    gint setup(GError** error);
    gboolean focusImageCaptured(GstElement* fsink, GstBuffer* buffer, GstPad* pad, gpointer user_data);
    gboolean setFocusLock();
    void focusAchieved();
    void setScanning (gboolean value, guint timeout);
    static gboolean releaseFocusLockWrapper(gpointer user_data);
    static gboolean focusTriggerWrapper(gpointer user_data);
    static gboolean runFocusWrapper(gpointer user_data);

private:
    CDAF focus_machine_;
    AdditionsParent* additions_parent_;

    gboolean grab_focus_frame_;
    guint focus_frame_timeout_;
    gboolean focussed_;
    gboolean focussing_;
    gboolean focus_lock_;
    gboolean scanning_;    
    gfloat focus_value_;
    gfloat focussed_value_;
    guint resolution_width_;
    guint resolution_height_;

    
    gboolean releaseFocusLock(gpointer user_data);
    //gboolean focusImageCaptured(GstElement* fsink, GstBuffer* buffer, GstPad* pad, gpointer user_data);
    gboolean runFocus(gpointer user_data); //Will need a wrapper as this is a g_idle_add callback
    gboolean focusTrigger(gpointer user_data); //Will need a wrapper as this is a g_idle_add callback

     /* At the moment trigger_focus_capture simply sets a flag to tell focus_image_captured
    * to process a frame, which will normally result in setting an idleRunFocus callback
    * meaning the camera gets a new focus setting.
    */

    void triggerFocusCapture();

    /* When a frame is set process in focus_image_captured, laplacianMean is called to
    * calculate the actual focus value of the frame. If the focus is set, then the focus value
    * has to differ from the set focus value in order to attempt focus again. If the system is 
    * trying to focus, then the new value is sent straight through to the algorithm.
    */
    gfloat laplacianMean (GstMapInfo *padinfo);


    /* GstElement *camera is the pipeline. All elements will need to be added to the camera pipeline.
    *  app->ele.camera = gst_pipeline_new ("capture_native_pipeline");; is at the top of the
    * static gboolean create_csi_capture_pipeline (void) function.
    * The piece of the pipeline is created by create_csi_cap_bin(). Here app->ele.capbin is created.
    * The capbin links app->ele.vsrc to app->ele.cap_filter and then sets ghost pads on app->ele.capbin
    * from the sink end of app->ele.cap_filter to be the capbin source for the next link in the chain.    
    */

    /*static gboolean create_focus_scaling_bin (void); is an inbetween
    * piece of the AF pipeline. The src pad hooks onto the capbin element
    * and its sinkpad in the source pad for static gboolean create_focus_enc_bin (void);
    *
    static gboolean create_focus_scaling_bin (void);*/

    /*static gboolean create_focus_enc_bin (void); is the end of the AF pipeline
    *This uses the focus scaling bin sink pad as its source, and sinks to fakesink.
    * Fakesink sets a callback which is called for each frame. Ideally fakesink is only
    * sent specific frames, such as with nvTee in image capture mode.
    *
    static gboolean create_focus_enc_bin (void);*/

    /*focus_image_captured is the fakesink callback set in focus enc bin
    * At the moment it gets every frame, but hopefully we can set things up
    * to send only selected frames through. This would be much prefferred.
    *
    static void focus_image_captured(GstElement* fsink,
    GstBuffer* buffer, GstPad* pad, gpointer udata); */
};

#endif //ADDITIONSFORAF_H