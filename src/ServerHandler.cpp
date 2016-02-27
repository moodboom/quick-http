#include <oauth/urlencode.h>                            // for urlencode()
#include "ServerHandler.hpp"


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------

bool APIGetLog::handle_call(const string& type, reply& rep)
{
    string rawlog = read_file(g_base_log_filename + ".log");

    if (strings_are_equal(type,"txt"))
    {
        rep.content = rawlog;
        return true;

    } else if (strings_are_equal(type,"html"))
    {
        // Add <br>.
        string log;
        stringstream ss(rawlog);
        string line;
        while (getline(ss, line))
            log += line + "<br />";

        // Inject the log into our static html.
        // NOTE that this uses "replace" from utilities.hpp.
        replace(static_html_, "<!-- /v1/log.html log goes here -->", log);
        rep.content = static_html_;

        return true;
    }
    return false;
}


bool APIGetUsers::handle_call(const string& type, reply& rep)
{
    // TODO
    return false;
}


bool APIGetUser::handle_call(const string& type, reply& rep)
{
    // TODO
    return false;
}

