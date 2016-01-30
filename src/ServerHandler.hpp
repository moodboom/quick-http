#pragma once

#include <basic_types.hpp>          // ALWAYS MAKE SURE THIS INCLUDE COMES FIRST
                                    // or you will get these ugly types of errors: ‘vector’ does not name a type
#include "Controller.hpp"
#include <http/http_helpers.hpp>


// Base API call that includes a reference to our controller.
class MyApiCall : public API_call
{
typedef API_call inherited;

public:
    MyApiCall(
        Controller& c,
        string method,
        vector<string> path_tokens,
        vector<string> types,
        vector<pair<string,string>> pair_tokens = vector<pair<string,string>>(),
        bool b_param_pairs_are_mandatory = true
    ) :
        inherited(method,path_tokens,types,pair_tokens,b_param_pairs_are_mandatory),
        c_(c)
    {}

protected:
    Controller& c_;
};


// -----------------
// API call handlers
// -----------------
class APIGetLog             : public MyApiCall { using MyApiCall::MyApiCall; public: virtual bool handle_call(reply& rep); };
class APIGetUsers           : public MyApiCall { using MyApiCall::MyApiCall; public: virtual bool handle_call(reply& rep); };
class APIGetUser            : public MyApiCall { using MyApiCall::MyApiCall; public: virtual bool handle_call(reply& rep); };
