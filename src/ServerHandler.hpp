#pragma once

#include <basic_types.hpp>          // ALWAYS MAKE SURE THIS INCLUDE COMES FIRST
                                    // or you will get these ugly types of errors: ‘vector’ does not name a type
#include "Controller.hpp"
#include <http/http_helpers.hpp>


// -----------------
// API call handlers
// -----------------
class APIGetLog             : public API_call { using API_call::API_call; public: virtual bool handle_call(reply& rep); };
class APIGetUsers           : public API_call { using API_call::API_call; public: virtual bool handle_call(reply& rep); };
class APIGetUser            : public API_call { using API_call::API_call; public: virtual bool handle_call(reply& rep); };


class server_handler : public base_server_handler
{
	typedef base_server_handler inherited;

public:

	explicit server_handler(
		Controller& c
	);

private:

	Controller& c_;
};


