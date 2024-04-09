#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <glib.h>

class AdditionsParent;

class ErrorHandler {
public:
    ErrorHandler(AdditionsParent* additions_parent);
    ~ErrorHandler();
    void errorHandler(GError** error);

private:
    AdditionsParent* additions_parent_;

    gboolean error_during_setup_;

};
#endif //ERRORHANDLER_H