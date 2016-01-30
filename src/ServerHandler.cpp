#include <oauth/urlencode.h>                            // for urlencode()
#include "ServerHandler.hpp"


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------

bool APIGetLog::handle_call(reply& rep)
{
    // Inject the log into our static html.
    // NOTE that this uses "replace" from utilities.hpp.
    string rawlog = read_file(g_base_log_filename + ".log");

    // Add <br>.
    string log;
    stringstream ss(rawlog);
    string line;
    while (getline(ss, line))
        log += line + "<br />";

    replace(static_html_, "<!-- /v1/log.html log goes here -->", log);
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

