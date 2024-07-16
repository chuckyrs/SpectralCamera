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

#ifndef ADDITIONSPARENT_C_H
#define ADDITIONSPARENT_C_H

#include <glib.h>
#include <gst/gst.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TriggerImageCapture)();

typedef void (*FocusValveOpen)();
typedef void (*FocusValveClose)();

typedef void (*AdditionsExitCapture)(GError**);

typedef struct AdditionsParent AdditionsParent;

/*These create and destroy the additions objects. All objects beneath additions_parent are built
* by the cunstructors (without using 'new' and so should be automatically destroyed when additions
* parent is destoryed. Cleaning up is done by each object's destructor. Therefore memory management
* should be well contained.
*/
AdditionsParent* additions_parent_create(GMainContext* main_context, gint* width, gint* height,
TriggerImageCapture trigger_image_capture, AdditionsExitCapture additions_exit_capture,
FocusValveOpen focus_valve_open, FocusValveClose focus_valve_close, GError** error);
void additions_parent_destroy(AdditionsParent* obj);

/*These functions are set as GSource callback functions*/
gboolean focusImageCaptured_C(GstElement* fsink, GstBuffer* buffer, GstPad* pad, gpointer user_data);
gboolean runSetup_C(gpointer data);

//Below function is called directly from C code 
void getImageFileName_C(AdditionsParent* obj, char* outfile);
#ifdef __cplusplus
}
#endif

#endif  // ADDITIONSPARENT_C_H




