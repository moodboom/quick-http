# quick-http
An http[s] client and server app skeleton in modern portable C++, focused on easy management of RESTful and websockets APIs, and high performance through a delayed-write model used to serve dynamic html completely from memory.

I have used this project as a starting point for several others.  It's not completely polished yet but well on its way.  It captures patterns and best practices that have worked well for me in production.  These include:

* Model View Controller pattern
* event-driven primary message loop with async multithreaded support (based on boost ASIO)
* hashmaps (unordered sets of pointers) used for all major object collections
* use of an auto-incremented id as the primary key for all objects
* support for secondary hash sorting on any desired object fields
* ability to do all object management in memory, including generation of primary key without hitting database
* in-memory base model storage layer; derived sqlite model storage layer implementation, with delayed write of dirty objects during idle time
* preloading of all static html assets into memory, including javascript and css

The base RESTful classes do the heavy lifting, so your derived class just provides the static headers, API format, and call handlers.  Example:
```
const vector<string> c_includes =
{
    "favicon.ico",
    "bootstrap/css/bootstrap.min.css",
    "css/grid.css",
    "css/sticky-footer-navbar.css",
    "css/main.css",
    "bootstrap/assets/js/jquery.min.js",
    "bootstrap/js/bootstrap.min.js",
};

const vector<API_call*> c_vpAPI =
{
    new APIGetLog         ( "GET"   , {"v1","log"                     }, {"html","json"} ),
    new APIGetStocks      ( "GET"   , {"v1","stocks"                  }, {"html","json"} ),
    new APIGetStock       ( "GET"   , {"v1","stocks",":ABCD"          }, {"html","json"} ),
    new APIGetParameters  ( "GET"   , {"v1","parameters"              }, {"html","json"} ),
    new APIGetParameter   ( "GET"   , {"v1","parameters",":id"        }, {"html","json"} ),
    new APIGetAccounts    ( "GET"   , {"v1","accounts"                }, {"html","json"} ),
    new APIGetAccount     ( "GET"   , {"v1","accounts",":id"          }, {"html","json"} ),
    new APIGetAccountChart( "GET"   , {"v1","accounts",":id","chart"  }, {"html","json"} ),
    new APIPostAccountPick( "POST"  , {"v1","accounts",":id","pick"   }, {"html","json"}, { pair<string,string>("symbol","[ABCD]"),pair<string,string>("active","[true|false]")      } ),
    new APIGetUsers       ( "GET"   , {"v1","users"                   }, {"html","json"} ),    // ADMIN
    new APIGetUser        ( "GET"   , {"v1","users",":id"             }, {"html","json"} ),
};

at_server_handler::at_server_handler()
:
    // call base class
    inherited(
        c_includes,
        c_vpAPI,
        1000000         // Allow max of 1MB requests for now
    )
{
}

// The static html for this function was automatically loaded from [htdocs/v1/log.html], 
// by the base class, which followed the path of the API format.
bool APIGetLog::handle_call(reply& rep)
{
    // Inject the log into our static html.
    string rawlog = get_log_contents();
    replace(static_html_, "<!-- /v1/log.html log goes here -->", rawlog);
    rep.content = static_html_;

    return true;
}
```

This project provides an example starting point for you to create your own API server.
To start:

* get the [moodboom/Reusable] project, it provides the base classes for this project
* search and replace all instances of my_quick_http_app with your application name - do a case-insensitive search with and without separators
* replace the example Car and Tire classes with your model:
** update MyApplicationModel.* with classes representing your model
** replace the RESTful API specification and handlers in ServerHandler.cpp

To build:
* for Visual Studio, CMake, or any other sane build tool, create a simple C++ project and add the modules listed in nix/copy_from/Makefile_src.am
* for autotools building:
** in nix/copy_from/Makefile_src.am, adjust the relative path to Reusable in nix/copy_from/Makefile_src.am
** set ENV vars as described in nix/bootstrap.sh; cd nix; ./bootstrap.sh force release debug
* for Eclipse:
** There are project files in the repo, but braindead Eclipse doesn't put all project settings into project settings files.  The provided ones should be a good start, though.

The project depends on the code in another project [moodboom/Reusable] that holds a large amount of reusable code that has been built up over years.  There is room for cleanup and improvement of style, etc. in the codebase - for example there is both snake_case and camelCase.  But the code itself is clean and the functionality has been tested in several projects.

This project uses code or inspiration from boost ASIO, twitter bootstrap and oath, eidheim/Simple-Web-Server, SQLiteCPP, etc.
