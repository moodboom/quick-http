#include "Controller.hpp"

#include <oauth/urlencode.h>							// for urlencode()
#include <oauth/oauthlib.h>								// for authenticating
#include <RandomHelpers.hpp>

#include "ServerHandler.hpp"

using namespace std;


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------
const vector<string> c_includes =
{
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
const vector<string> c_wrappers =
{
    "<form class=\"api-form\" method=\"__API_method__\" action=\"__API_url__\"><div class=\"form-inline\">",                        // HW_LINE_BEGIN
    "</div></form>",                                                                                                                // HW_LINE_END
    "<button type=\"submit\" class=\"btn btn-fixed btn-__API_method__\">__API_method__</button><div class=\"form-group\"></div>",   // HW_METHOD
    "<label>__API_token__</label>",                                                                                                 // HW_PATH
    " <input type=\"text\" name=\"__API_token__\" class=\"form-control\" placeholder=\"__API_token__\"/> "                          // HW_PARAM
};
// ------------------------------------------------------------------------------


Controller::Controller(
    MemoryModel& mm,
	string str_host,
	string str_port,
    bool b_test
) :
	// Init vars.
    memory_model_(mm),
	str_host_(str_host),
	str_port_(str_port),
    b_test_(b_test),
    timer_(io_service_),
    analysis_timer_(io_service_),
	b_run_analysis_(false),

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
	// Note that PATCH technically requires "specification of a sequence of operations".  I dunno, that's rather complex.
	// Better to add an additional GET/PUT path under the parent for each item needing direct change.
	//  http://williamdurand.fr/2014/02/14/please-do-not-patch-like-an-idiot/
	//  http://tools.ietf.org/html/rfc6902
	// =====================================================================================
	API_({
        new APIGetLog           ( *this, HM_GET   , {"v1","log"                                 }, {"html","json"} ),
        new APIGetAccounts      ( *this, HM_GET   , {"v1","accounts"                            }, {"html","json"} ),
        new APIPostAccount      ( *this, HM_POST  , {"v1","accounts"                            }, {"json"}        ),
        new APIGetAccount       ( *this, HM_GET   , {"v1","accounts",":id"                      }, {"html","json"} ),
        new APIPutAccount       ( *this, HM_PUT   , {"v1","accounts",":id"                      }, {"json"}        ),
        new APIDeleteAccount    ( *this, HM_DELETE, {"v1","accounts",":id"                      }, {"json"}        ),
        new APIGetPortfolios    ( *this, HM_GET   , {"v1","accounts",":id","portfolios"         }, {"html","json"} ),
        new APIPostPortfolio    ( *this, HM_POST  , {"v1","accounts",":id","portfolios"         }, {"json"}        ),
        new APIGetPortfolio     ( *this, HM_GET   , {"v1","accounts",":id","portfolios",":id"   }, {"html","json"} ),
        new APIPutPortfolio     ( *this, HM_PUT   , {"v1","accounts",":id","portfolios",":id"   }, {"json"}        ),
        new APIDeletePortfolio  ( *this, HM_DELETE, {"v1","accounts",":id","portfolios",":id"   }, {"json"}        ),
	})

{
    g_p_local = &memory_model_;
}


int Controller::run()
{
   // Do we have handshaking with other systems to do before beginning the main timer loop?
   // If so use something like this.
   /*
   bool b_ready = false;
   while (!b_ready)
   {
      b_ready = load_system_settings();
      if (!b_ready)
      {
         log(LV_WARNING,"System not ready yet, retrying in 15 seconds...");
         sleep(15);
      }
   }
   */

   if (!load_startup_data())
   {
       log(LV_ERROR,"load_startup_data() failed, exiting...");
       return 1;
   }

   start_server();

   return 0;
}


bool Controller::load_startup_data()
{
    if (!g_p_local->initialize(b_test_))
    {
        log(LV_ERROR,"local model did not initialize...");
        return false;
    }

    if (bTest())
    {
        load_test_data();

        // No need to authenticate or refresh.
        return true;
    }

    if (!authenticate_services())
    {
        log(LV_ERROR,"authenticate_services() failed...");
        return false;
    }

    return true;
}


bool Controller::load_test_data()
{
    // Not sure we will have to write,
    // but it's ok to have one transaction overhead on app startup.
    // Better than a bunch of smaller transactions.
    g_p_local->startTransaction();

    // Test.

    g_p_local->saveDirtyObjectsAsNeeded();

    g_p_local->endTransaction();

    return true;
}


bool Controller::authenticate_services()
{
	// TODO

    return true;
}


void Controller::start_server()
{
    // We are ready to let our io_service_ take over.
    // We chain several async handlers to it:
    //
    //      http_server, our server with a custom handler for requests (see handler for details)
    //      start a timer to regularly run analysis
    //      listen for SIGTERM so user can exit if needed
    //

    // log(LV_ALWAYS,"Starting http server...");

    // HTTP SERVER
    server_handler sh(
        c_includes,
        API_,
        "My Quick Http App",
        c_wrappers
    );
    Server<HTTP> s(
		io_service_,
		sh,
		str_host_,
		str_port_,
		4					// # threads, TODO make this smarter
	);

    // SIGTERM
    // Make the server listen for SIGTERM.
    // Use our own stop function, so we can exit all async waits.
	boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);
	signals.async_wait(boost::bind(&Controller::handle_stop, this));

    // STARTUP DELAY TIMER
    timer_.expires_from_now(boost::posix_time::milliseconds(g_p_local->get_pref(IP_STARTUP_DELAY_MS)));           // wait a beat
    timer_.async_wait(boost::bind(&Controller::startup_delay_finished,this,boost::asio::placeholders::error));

    // Run the server until stopped.
    // log(LV_ALWAYS,"Listening for RESTful requests...");
	s.run();

}


void Controller::handle_stop()
{
    log(LV_ALWAYS,"Stopping as requested...");
    timer_.cancel();
    analysis_timer_.cancel();
    io_service_.stop();
}


// NOTE that there is no return value.  If we don't succeed, we prime the timer to retry again later.
void Controller::startup_delay_finished(const boost::system::error_code& error)
{
    // WARNING: ALWAYS PROVIDE AND HANDLE THE ERROR PARAM in any timer callbacks.
    // You can define a callback without it, 
    // but boost will CALL THE CALLBACK unexpectedly on you, 
    // with error set to a positive value, when timers are canceled.  Bad boost!
    if (error)
        return;

	log(LV_ALWAYS,"Starting main loop...");

	// Also start the analysis timer to get the "second" request, which begins the timeout loop.
	analysis_timer_.expires_from_now(boost::posix_time::milliseconds(g_p_local->get_pref(IP_RUN_ANALYSIS_FREQUENCY_MS)));
	analysis_timer_.async_wait(boost::bind(&Controller::ready_for_run_analysis,this,boost::asio::placeholders::error));

    // We're now ready to immediately start the main loop.  Use a delay of 0, then wait.
    timer_.expires_from_now(boost::posix_time::milliseconds(0));
    timer_.async_wait(boost::bind(&Controller::main_loop,this,boost::asio::placeholders::error));
}


// ---------------------------------
//            MAIN LOOP
// ---------------------------------
void Controller::main_loop(const boost::system::error_code& error)
{
    // WARNING: ALWAYS PROVIDE AND HANDLE THE ERROR PARAM in any timer callbacks.
    // You can define a callback without it, 
    // but boost will CALL THE CALLBACK unexpectedly on you, 
    // with error set to a positive value, when timers are canceled.  Bad boost!
    if (error)
        return;


    // ------------------------------------------------
    // Handle triggered events
    // ------------------------------------------------
    // Perform tasks that should happen on every loop.
    // This is where we respond as quickly as possible.
    // Typically this is used to handle any type of externally-triggered event.
    // ------------------------------------------------
    if (api_requests_.b_received_NewTires())
    {
        vector<NewTire> v;
        api_requests_.extract_NewTires(v);
        for (auto itnd = v.begin(); itnd != v.end(); ++itnd)
            API_receive_NewTire(*itnd);
    }
    // ------------------------------------------------


    // ------------------------------------------------
    // Handle timer events
    // ------------------------------------------------
    // Here we handle events that occur at regular intervals.
    //
    // Rather than trying to coordinate multiple message loops,
    // use this "infrequent" pattern to fire off infrequent tasks
    // from within the more-frequent message loop.
    // Pretty much anything we do on a "regular" basis should use
    // this built-in delay cycle.  This helps reduce thread count and mutex locking.
    // ------------------------------------------------
    if (b_run_analysis_)
    {
        // DEBUG
        // How often do we call?  How long does it take?
        uint64_t start_time;
        start_profile_ms(start_time);

        run_analysis();

        end_profile_ms(start_time,"ANALYSIS TOOK");
    }
    // ------------------------------------------------


    // Prime the next loop.
    timer_.expires_from_now(boost::posix_time::milliseconds(g_p_local->get_pref(IP_MAIN_LOOP_RATE_MS)));
    timer_.async_wait(boost::bind(&Controller::main_loop,this,boost::asio::placeholders::error));
}


void Controller::API_receive_NewTire(const NewTire& np)
{
    // TODO add appuser + ... to API call
	// g_p_local->addTire(new Tire(np.name_, np.type_);
}


void Controller::run_analysis()
{
    if (bTest())
    {
    	// TODO

    } else
    {
    	// TODO
    }

    sleep(1+get_random_uint32(5));

    g_p_local->saveDirtyObjectsAsNeeded();

    // Don't trigger another until we are done.
    // We may have changed the timer above.
    b_run_analysis_ = false;
}
void Controller::ready_for_run_analysis(const boost::system::error_code& error)
{
    // WARNING: ALWAYS PROVIDE AND HANDLE THE ERROR PARAM in any timer callbacks.
    // You can define a callback without it,
    // but boost will CALL THE CALLBACK unexpectedly on you,
    // with error set to a positive value, when timers are canceled.  Bad boost!
    if (error)
        return;

    b_run_analysis_ = true;

    analysis_timer_.expires_from_now(boost::posix_time::milliseconds(g_p_local->get_pref(IP_RUN_ANALYSIS_FREQUENCY_MS)));
    analysis_timer_.async_wait(boost::bind(&Controller::ready_for_run_analysis,this,boost::asio::placeholders::error));
}


