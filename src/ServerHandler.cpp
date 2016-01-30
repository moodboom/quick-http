#include <oauth/urlencode.h>                            // for urlencode()
#include "ServerHandler.hpp"


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------

bool APIGetLog::handle_call(reply& rep)
{
    // Inject the log into our static html.
    string rawlog = read_file(g_base_log_filename + ".log");
    replace(static_html_, "<!-- /v1/log.html log goes here -->", rawlog);
    rep.content = static_html_;

    return true;
}


bool APIGetUsers::handle_call(reply& rep)
{
    // TODO
    return false;
}


bool APIGetUser::handle_call(reply& rep)
{
    // TODO
    return false;
}

