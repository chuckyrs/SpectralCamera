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

ErrorHandler::ErrorHandler(AdditionsParent* additions_parent):
error_during_setup_(FALSE),
additions_parent_(additions_parent) {
        g_print ("...Error handler\n");
}

ErrorHandler::~ErrorHandler() {
        g_print ("Shutting error handler\n");
}
 
void ErrorHandler::errorHandler(GError** error) {
        additions_parent_->errorShutdown(error);
}