#pragma once

#include <basic_types.hpp>
#include <utilities.hpp>

#include "MemoryModel.hpp"


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------
class NewTire
{
public:
    string name_;
    string type_;
};
// ------------------------------------------------------------------------------


class APIRequestCache
{
public:
    APIRequestCache() 

    /*
    :
        // init vars
        b_received_some_new_bool_setting_(false)        <-- you can use a bool member if you need to track a single bool state's toggle
    */

    {}

    // MDM no chunk events at this time
    /*
    void receive_event(const ChunkEvent& chunk_event);
    bool b_events();
    void extract_events(vector<ChunkEvent>& chunk_events);
	*/

    void receive_NewTire(const NewTire& nd);
    bool b_received_NewTires();
    void extract_NewTires(vector<NewTire>& vNewTires);

private:

    // MDM no chunk events at this time
    // boost::shared_mutex events_guard_;
    // vector<ChunkEvent> chunk_events_;

    boost::shared_mutex NewTire_guard_;
    vector<NewTire> vNewTires_;
};
