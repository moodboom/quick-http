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
        rep.content = static_html_;
        replace(rep.content, "<!-- /v1/log.html log goes here -->", log);

        return true;
    }
    return false;
}


bool APIGetAccounts    ::handle_call(const string& type, reply& rep) { rep.content = static_html_; return true; }
bool APIPostAccount    ::handle_call(const string& type, reply& rep) { rep.content = static_html_; return true; }
bool APIGetAccount     ::handle_call(const string& type, reply& rep) { rep.content = static_html_; return true; }
bool APIPutAccount     ::handle_call(const string& type, reply& rep) { rep.content = static_html_; return true; }
bool APIDeleteAccount  ::handle_call(const string& type, reply& rep) { rep.content = static_html_; return true; }
bool APIGetPortfolios  ::handle_call(const string& type, reply& rep) { rep.content = static_html_; return true; }
bool APIPostPortfolio  ::handle_call(const string& type, reply& rep) { rep.content = static_html_; return true; }
bool APIGetPortfolio   ::handle_call(const string& type, reply& rep) { rep.content = static_html_; return true; }
bool APIPutPortfolio   ::handle_call(const string& type, reply& rep) { rep.content = static_html_; return true; }
bool APIDeletePortfolio::handle_call(const string& type, reply& rep) { rep.content = static_html_; return true; }
