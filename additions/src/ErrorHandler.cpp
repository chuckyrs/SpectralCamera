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


// Need to check which includes are actually required.
#include <sys/stat.h>
#include <sstream>
#include <iomanip>
#include <memory>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>
#include <dirent.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <memory>

#include "ErrorHandler.h"
#include "AdditionsParent.h"

/**
 * Constructs an ErrorHandler object associated with a parent object handling additions.
 *
 * @param additions_parent : Pointer to the AdditionsParent object that this ErrorHandler will interact with.
 *                           AdditionsParent passes errors back nvgstcapture-1.0.
 */
ErrorHandler::ErrorHandler(AdditionsParent* additions_parent):
error_during_setup_(FALSE), // Initializes error flag to FALSE indicating no error during setup.
additions_parent_(additions_parent) { // Stores the pointer to the AdditionsParent object.
        g_print ("...Error handler\n"); // Logs initialization message.
}


/**
 * Destructor for ErrorHandler. Cleans up resources and logs the shutdown process.
 */
ErrorHandler::~ErrorHandler() {
        g_print ("Shutting error handler\n"); // Logs shutdown message.
}

/**
 * Handles errors by delegating to the error handling mechanism of the associated AdditionsParent object.
 *
 * @param error : Double pointer to a GError structure to allow error information to be passed in and modified.
 *                If an error occurs in the handling process, the pointed GError will be updated accordingly.
 */
void ErrorHandler::errorHandler(GError** error) {
        additions_parent_->errorShutdown(error); // Invokes the errorShutdown method of the AdditionsParent object.
                                                 // The error passes through to nvgstcapture-1.0
}