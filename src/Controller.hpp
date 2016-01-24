#pragma once

#include <basic_types.hpp>
#include <utilities.hpp>
#include <http/quick_http_server.hpp>

#include "MemoryModel.hpp"
#include "UserInterface.hpp"
#include "APIRequestCache.hpp"


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------


class Controller
{
public:

	Controller(
	    MemoryModel& mm,
		std::string str_host,
		std::string str_port,
        bool b_test
	);

	int run();

    bool bTest() const { return b_test_; }

    // These are called after complete events are received.
    // void handle_operation(Operation& operation);

    boost::asio::io_service io_service_;

    APIRequestCache api_requests_;

private:

    bool load_startup_data();
    bool load_test_data();
    bool authenticate_services();
    void start_server();

    void API_receive_NewTire(const NewTire& np);

    void run_analysis();

    // Timer-driven callback functions.
    void startup_delay_finished(const boost::system::error_code& error);
    void main_loop(const boost::system::error_code& error);
    void ready_for_run_analysis(const boost::system::error_code& error);
    void handle_stop(); // Full stop, both http and timers

    // These should get pushed down to derived layers if/when they exist.
    // Then these members should become references assigned in the constructor.
    // See the hangthedj Qt app for a more detailed example.
    MemoryModel& memory_model_;
    UserInterface ui_;

    string str_host_;
	string str_port_;

    bool b_test_;

    boost::asio::deadline_timer timer_;

    boost::asio::deadline_timer analysis_timer_;
    bool b_run_analysis_;
};

