#include <oauth/urlencode.h>                            // for urlencode()
#include "ServerHandler.hpp"


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------

const vector<string> c_includes =
{
    "favicon.ico",
    "bootstrap/css/bootstrap.min.css",
    "bootstrap/assets/css/ie10-viewport-bug-workaround.css",
    "css/grid.css",
    "css/sticky-footer-navbar.css",
    "css/main.css",
    "css/bootstrap-switch.min.css",
    "bootstrap/assets/js/jquery.min.js",
    "bootstrap/js/bootstrap.min.js",
    "js/bootstrap-switch.min.js",
    "bootstrap/assets/js/ie10-viewport-bug-workaround.js"
};

// =====================================================================================
//  API DESIGN GUIDELINES
// =====================================================================================
// Keep the RESTful design tight!
//  https://en.wikipedia.org/wiki/Representational_state_transfer#RESTful_web_services
//
//                                              GET                 PUT                         POST                                    DELETE              PATCH
//      Collection  http://a.com/v1/things/     list                n/a (replace set)           create new element, return URI to it    delete whole set    n/a
//      Element     https://a.com/v1/things/3   element n details   create/replace element n    n/a                                     delete element n    change part of element n
//
// Rule of thumb is 4xx errors indicate the CLIENT did something incorrectly
// while 5xx errors indicate the SERVER did something incorrectly.
// =====================================================================================
const vector<API_call*> c_vpAPI =
{
    new APIGetLog         ( "GET"   , {"v1","log"                                           }, {"html","json"} ),
    new APIGetUsers       ( "GET"   , {"v1","users"                                         }, {"html","json"} ),    // ADMIN
    new APIGetUser        ( "GET"   , {"v1","users",":id"                                   }, {"html","json"} ),
};

// ------------------------------------------------------------------------------


server_handler::server_handler(
    Controller& c
) :
    // call base class
        // TODO add a string to inject as the Title (ie, A better Trader)
    inherited(
        c_includes,
        c_vpAPI,
        1000000         // Allow max of 1MB requests for now
    ),

    // init vars
    c_(c)
{
}


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

