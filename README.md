# quick-http

> An http[s] client and server app skeleton in modern portable C++, 
> focused on easy management of RESTful and websockets APIs, 
> and high performance through a delayed-write model used to serve dynamic html completely from memory.

![Self-documentation example](REST_selfdoc_screenshot.png?raw=true "Self-documentation example")

### Usage

Provide static headers, API format, and call handlers like this:
```
const vector<string> c_includes =
{
    "favicon.ico",
    "bootstrap/css/bootstrap.min.css",
    "css/main.css",
    "bootstrap/assets/js/jquery.min.js",
    "bootstrap/js/bootstrap.min.js",
};

Controller::Controller()
) :
    API_({
        new APIGetLog       ( *this, HM_GET   , {"v1","log"            }, {"html","json"} ),
        new APIGetUsers     ( *this, HM_GET   , {"v1","accounts"       }, {"html","json"} ),
        new APIPostUser     ( *this, HM_POST  , {"v1","accounts"       }, {"json"}        ),
        new APIGetAccounts  ( *this, HM_GET   , {"v1","accounts",":id" }, {"html","json"} ),
        new APIPutAccounts  ( *this, HM_PUT   , {"v1","accounts",":id" }, {"json"}        ),
        new APIDeleteAccount( *this, HM_DELETE, {"v1","accounts",":id" }, {"json"}        ),
        // (etc)...
    })
{}

bool APIGetLog::handle_call(reply& rep)
{
    // Inject the log into our static html.
    string rawlog = get_log_contents();
    replace(static_html_, "<!-- /v1/log.html log goes here -->", rawlog);
    rep.content = static_html_;

    return true;
}
```

### Patterns

This skeleton app follows these patterns and best practices:

✓ model-view-controller pattern  
✓ event-driven primary message loop with async multithreaded support (via boost ASIO)  
✓ efficient memory model  
  * hashmaps (unordered sets of pointers) used for all major object collections
  * support for secondary hash sorting on any desired object fields
  * use of an auto-incremented id as the primary key for all objects
  * ability to do all object management in memory, including generation of new unique ids without hitting database
  * in-memory base model storage layer; derived sqlite model storage layer implementation, with delayed write of dirty objects during idle time  

✓ one html codebase  

  * html files are directly browseable and editable from the file system, for instantaneous web development
  * html files include test data that is used when browsing from the file system
  * html files are fully preloaded into memory on startup, including injection of javascript and css, and removal of test data
  * during runtime, dynamic data can be injected into preloaded html, for delivery from memory of full html pages in one round trip

See the [wiki](https://bitpost.com/wiki/Quick-http) for performance stats and other information.

### Getting started

This project provides an example starting point for you to create your own API server.  To start:

* get the [moodboom/Reusable] project, it provides the base classes for this project
* search and replace all instances of my_quick_http_app with your application name - do a case-insensitive search with and without separators
* replace the example Car and Tire classes with your model:
  * update MyApplicationModel.* with classes representing your model
  * replace the RESTful API specification and handlers in ServerHandler.cpp

To build:
* for Visual Studio, CMake, or any other sane build tool, create a simple C++ project and add the modules listed in nix/copy_from/Makefile_src.am
* for autotools building:
  * in nix/copy_from/Makefile_src.am, adjust the relative path to Reusable in nix/copy_from/Makefile_src.am
  * set ENV vars as described in nix/bootstrap.sh; cd nix; ./bootstrap.sh force release debug
* for Eclipse:
  * There are project files in the repo, but braindead Eclipse doesn't put all project settings into project settings files.  The provided ones should be a good start, though.

The project depends on the code in another project [moodboom/Reusable] that holds a large amount of reusable code that has been built up over years.  There is room for cleanup and improvement of style, etc. in the codebase - for example there is both snake_case and camelCase.  But the code itself is clean and the functionality has been tested in several projects.

This project uses code or inspiration from boost ASIO, twitter bootstrap and oath, eidheim/Simple-Web-Server, SQLiteCPP, etc.

