// ======================================
//     ~~~~~~~~~ MyQuickHttpApp ~~~~~~~~~
// ======================================
//
// Purpose
//
//
// Requirements
//
//
// ======================================

#include "SqliteLocalModel.hpp"
#include "Controller.hpp"
#include "Version.hpp"


void usage();


using namespace std;
int main( int argc, char * argv[] )
{
    /*
	// We can find "our" address with this.
	// BUT... it gets complicated.  There may be multiple NICs,
	// there may be LAN and WAN addresses, wired and wireless, loopback...
    // on desktop i had one NIC but two ips here from two diff DHCP servers on my lan...
	// Better to let the user provide as a parameter.
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");
	boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
	boost::asio::ip::tcp::resolver::iterator end; // End marker.
	while (iter != end)
	{
		boost::asio::ip::tcp::endpoint ep = *iter++;
		std::cout << ep << std::endl;
	}
    */

	try
	{
		// =======================================
		// Command line parameter handling
	    // =======================================
        // 
        // COMMAND LINE PARAMETERS INTERNAL VERSIONING
        // Update release notes on any additions or changes.
        //
        // v1   2015/10/22 initially created
        // 
		// =======================================
	    bool b_params_ok = false;

        std::string str_host;
		std::string str_port;
        bool b_test = false;

		if (argc >= 2)
		{
			str_host	= argv[1];
			str_port	= argv[2];

			b_params_ok =
			!(
                    str_host.length() > 700 || str_port.length() > 10
	    		|| 	str_host.length() <=  0 || str_port.length() <= 0
            );

            if (b_params_ok && argc > 2)
            {
                for (int arg_loop = 2; arg_loop < argc; ++arg_loop)
                {
                    if (string("test") == argv[arg_loop])
                    {
                        b_test = true;

                  	/*
                    } else if (string("second_function") == argv[arg_loop])
                    {
                        b_analysis = true;
                        b_params_ok = b_params_ok && (!b_test);

                    // Next optional param example when needed
                    } else if (string("limit") == argv[arg_loop])
                    {
                        b_limit = true;
         		        ++arg_loop; if (arg_loop < argc) start_dev .id_[0] = boost::lexical_cast<int>(argv[arg_loop]); 
                    */

                    }
                }
            }
        }

	    // Initialize logging globals.
	    g_base_log_filename = "my_quick_http_app";
	    g_current_log_verbosity = LV_INFO;

        archive_any_old_log_file();

        g_ss.str(string());
        g_ss << "========================================================"    << endl;
        g_ss << " my_quick_http_app v " << get_version_str() << " on port " << str_port     << endl;
        g_ss << "========================================================"    << endl;

        if (b_test)
        {
            g_ss << "   *** TEST MODE ***" << endl;
            g_ss << "========================================================"    << endl;
        }
		
        log(LV_ALWAYS,g_ss.str());

        if (!b_params_ok)
	    {
            usage();
			return 1;
		}
		// =======================================

        SqliteLocalModel slm;
		Controller c(slm,str_host,str_port,b_test);
		return c.run();

	}
	catch (std::exception& e)
	{
		std::cerr << "exception: " << e.what() << "\n";
	}

	return 0;
}


void usage()
{
	cerr << " "                                                                             << endl;
	cerr << "Usage:"	                                                                    << endl;
	cerr << "   my_quick_http_app host port [test]"                                         << endl;
    cerr << " "                                                                             << endl;
    cerr << " If [test] is provided, tests are run, then the executable will exit."         << endl;
    cerr << " "                                                                             << endl;
	cerr << "Examples:"                                                                    	<< endl;
	cerr << "  my_quick_http_app 192.168.0.23 80"			                                << endl;
    cerr << "  my_quick_http_app 192.168.1.3 8080 test"                                     << endl;
    cerr << "  my_quick_http_app badboy 443 test"                                           << endl;
	cerr << " "                                                                             << endl;
	cerr << "1) NOTE that you should provide a specific host or IP"                         << endl;
	cerr << "   for the [host] param, not localhost or 127.0.0.1,"                          << endl;
    cerr << "   if you are not running everything on localhost,"                            << endl;
    cerr << "   or if you want to connect by hostname."                                     << endl;
    cerr << " "                                                                             << endl;
}
