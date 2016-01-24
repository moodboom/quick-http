#include "op_controller.hpp"


void APIRequestCache::receive_event(const ChunkEvent& chunk_event)
{
    boost::upgrade_lock<boost::shared_mutex> lock(events_guard_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    // We may collect a bunch of events, keep gathering them up.
    chunk_events_.push_back(chunk_event);
}
bool APIRequestCache::b_events()
{
    boost::shared_lock<boost::shared_mutex> lock(events_guard_);
    return (chunk_events_.size() > 0);
}
void APIRequestCache::extract_events(vector<ChunkEvent>& events)
{
    boost::upgrade_lock<boost::shared_mutex> lock(events_guard_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    events = chunk_events_;
    chunk_events_.clear();
}


void APIRequestCache::receive_storm_mode(bool b_storm_mode)
{
    boost::upgrade_lock<boost::shared_mutex> lock(storm_mode_guard_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    b_received_new_storm_mode_ = true;
    b_storm_mode_ = b_storm_mode;
}
bool APIRequestCache::b_received_new_storm_mode()
{
    boost::shared_lock<boost::shared_mutex> lock(storm_mode_guard_);
    return b_received_new_storm_mode_;
}
bool APIRequestCache::extract_storm_mode()
{
    boost::upgrade_lock<boost::shared_mutex> lock(storm_mode_guard_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    b_received_new_storm_mode_ = false;
    return b_storm_mode_;
}


void APIRequestCache::receive_job_action(int_fast64_t job_id, string action)
{
    // OLD simple on/off mutex
    // boost::mutex::scoped_lock lock(job_action_guard_);                    

    boost::upgrade_lock<boost::shared_mutex> lock(job_action_guard_);           // get upgradable access
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);    // get exclusive access

    job_actions_.push_back(pair<int_fast64_t,string>(job_id,action));
}
bool APIRequestCache::b_job_actions()
{
    boost::shared_lock<boost::shared_mutex> lock(job_action_guard_);    // read access
    return (job_actions_.size() > 0);
}
void APIRequestCache::extract_job_actions(vector<pair<int_fast64_t,string> >& job_actions)
{
    boost::upgrade_lock<boost::shared_mutex> lock(job_action_guard_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    job_actions = job_actions_;
    job_actions_.clear();
}


void APIRequestCache::receive_operation(const DeviceOperation& dev_operation)
{
    boost::upgrade_lock<boost::shared_mutex> lock(operation_guard_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    dos_.push_back(dev_operation);  // NOTE that we should maintain order, no sorting
}
bool APIRequestCache::b_operations()
{
    boost::shared_lock<boost::shared_mutex> lock(operation_guard_);
    return (dos_.size() > 0);
}
void APIRequestCache::extract_operations(vector<DeviceOperation>& dos)
{
    boost::upgrade_lock<boost::shared_mutex> lock(operation_guard_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    dos = dos_;
    dos_.clear();
}


void APIRequestCache::receive_island_changes(const IslandSet& a,const IslandSet& r)
{
    boost::upgrade_lock<boost::shared_mutex> lock(island_guard_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    a_.insert(a.begin(), a.end());
    r_.insert(r.begin(), r.end());
}
bool APIRequestCache::b_island_changes()
{
    boost::shared_lock<boost::shared_mutex> lock(island_guard_);
    return (a_.size() > 0 || r_.size() > 0);
}
void APIRequestCache::extract_island_changes(IslandSet& a,IslandSet& r) 
{
    boost::upgrade_lock<boost::shared_mutex> lock(island_guard_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    a = a_;
    r = r_;
    a_.clear();
    r_.clear();
}


void APIRequestCache::receive_contact(const Contact& contact)
{
    boost::upgrade_lock<boost::shared_mutex> lock(contact_guard_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    contacts_.push_back(contact);  // NOTE that we should maintain order, no sorting
}
bool APIRequestCache::b_contacts()
{
    boost::shared_lock<boost::shared_mutex> lock(contact_guard_);
    return (contacts_.size() > 0);
}
void APIRequestCache::extract_contacts(vector<Contact>& contacts)
{
    boost::upgrade_lock<boost::shared_mutex> lock(contact_guard_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    contacts = contacts_;
    contacts_.clear();
}


// HANDLE EVENT
// Core sends us everything it sees through one chunked event stream.
// As http_chunked_client receives completed chunks (delineated by a byte count), 
// we receive a full event here for procesing.  This will be in the main thread,
// so we can perform our synchronous processing here.
void op_controller::handle_event(const ChunkEvent& event)
{
    try
    {
        // DEBUG
        // cout << "Received event: model [" << event.str_model_ << "] event type [" << event.str_event_type_ << "]..." << endl;

        //
        // Event order: apply storm mode, job actions, operate, apply island changes, add contacts
        //
        if (event.str_model_ == "Job")
        {
            if (event.str_event_type_ == "add")
            {
                // Do we have this job already?
                // If so do an update instead.

                // Add it
    /*
   cout << "in handle job action jobid: " << job_id << endl;

   if (action_str == "restore") {
      nwm_.restore_outage(job_id);
   } else if (action_str == "verify") {
      nwm_.verify_outage(job_id);
   } else {
      cout << "bad action code: " << action_str << endl;
   }

   post_outages();

    */

                
            } else if (event.str_event_type_ == "update")
            {

            }

        } else if (event.str_model_ == "OaOperation")
        {
            cout << "got OaOperation" << endl;
                try
                {
                    rapidjson::Document d;
                    d.Parse<0>(event.str_JSON_.c_str());
                    
                    int_fast64_t id = d["id"].GetInt64();
                    string oper_str = d["createdBy"].GetString();
                    string time_str = d["created"].GetString();
                    string intent_str = d["intentType"].GetString();

                    if (intent_str == "OaForceUpJobOperation") 
                    {
                       int_fast64_t job_id = d["jobId"].GetInt64();

                       nwm_.force_upstream(job_id, oper_str, time_str);

                    } else if (intent_str == "OaForceDownJobOperation") 
                    {
                       int_fast64_t job_id = d["jobId"].GetInt64();

                       nwm_.force_downstream(job_id, oper_str, time_str);
                    } else if (intent_str == "OaLockJobOperation") 
                    {
                       int_fast64_t job_id = d["jobId"].GetInt64();

                       nwm_.lock_outage(job_id, oper_str, time_str);
                    } else if (intent_str == "OaUnLockJobOperation") 
                    {
                       int_fast64_t job_id = d["jobId"].GetInt64();

                       nwm_.unlock_outage(job_id, oper_str, time_str);
                    } else if (intent_str == "OaVerifyJobOperation") 
                    {
                       int_fast64_t job_id = d["jobId"].GetInt64();

                       nwm_.verify_outage(job_id, oper_str, time_str);
                    } else if (intent_str == "OaRestoreJobOperation") 
                    {
                       int_fast64_t job_id = d["jobId"].GetInt64();

                       nwm_.restore_outage(job_id, oper_str, time_str);
                    } else if (intent_str == "OaCustomersVerifyCustomerOperation") 
                    {
                       vector <long> cust_set;

                       cout << "in OaCustomersVerifyCustomerOperation: list of objects" << endl;
                       const rapidjson::Value& val_nid = d["intentObjects"];
                       for (auto itn = val_nid.Begin(); itn != val_nid.End(); ++itn)
                       {
                           cout << itn - val_nid.Begin() << ": " << itn->GetString() << endl;
                           cust_set.push_back(std::stol(itn->GetString())); 
                       }

                       nwm_.create_verified_customer_outages(oper_str, time_str, cust_set);
                       nwm_.process_dirty_outages(JS_VERIFIED, NULL);

                    } else if (intent_str == "OaCustomersVerifySecondaryOperation") 
                    {

                       vector <long> cust_set;

                       cout << "in OaCustomersVerifySecondaryOperation: list of objects" << endl;
                       const rapidjson::Value& val_nid = d["intentObjects"];
                       for (auto itn = val_nid.Begin(); itn != val_nid.End(); ++itn)
                       {
                           cout << itn - val_nid.Begin() << ": " << itn->GetString() << endl;
                           cust_set.push_back(std::stol(itn->GetString())); 
                       }

                       nwm_.create_verified_secondary_outage(oper_str, time_str, cust_set);
                       nwm_.process_dirty_outages(JS_VERIFIED, NULL);

                    } else if (intent_str == "OaCustomersRestoreCustomerOperation") 
                    {
                       vector <long> cust_set;

                       cout << "in OaCustomersRestoreCustomerOperation: list of objects" << endl;
                       const rapidjson::Value& val_nid = d["intentObjects"];
                       for (auto itn = val_nid.Begin(); itn != val_nid.End(); ++itn)
                       {
                           cout << itn - val_nid.Begin() << ": " << itn->GetString() << endl;
                           cust_set.push_back(std::stol(itn->GetString())); 
                       }

                       nwm_.mark_customers_restored(oper_str, time_str, cust_set);
                       nwm_.process_dirty_outages(JS_RESTORED, NULL);

                       // get list of customerids
                       //nwm_.create_customer_outages_api(job_id, oper_str, time_str);
                    }

                    post_outages();

                } 
                catch(const std::exception& se)
                {
                    cout << "Error [" << se.what() << "] parsing oaoperation JSON: " << endl << event.str_JSON_ << endl;
                    return;
                }
        } else if (event.str_model_ == "Customer")
        {
            if (event.str_event_type_ == "add")
            {
                cout << "got customer add chunk" << endl;

            } else if ((event.str_event_type_ == "update") || (event.str_event_type_ == "replace"))
            {
                cout << "got customer update chunk" << endl;
                try
                {
                    rapidjson::Document d;
                    d.Parse<0>(event.str_JSON_.c_str());
                    
                    // NOTE that the id is provided in the event header, not in the body JSON.
                    // int_fast64_t id = d["id"].GetInt64();
                    int_fast64_t id = event.id_;

                    Customer *p_cust = nwm_.get_customer(id);
                    NetworkObjectID loadid;

                    if (!p_cust) {
                        cout << "Error retrieving customer for chunked update: " << endl << event.str_JSON_ << endl;
                        return;
                    }
                    const rapidjson::Value& val_nid = d["distXfmrUuid"];
                    for (auto itn = val_nid.Begin(); itn != val_nid.End(); ++itn)
                    {
                        loadid.id_[itn - val_nid.Begin()] = itn->GetInt64();
                    }

                    Load *p_newload = nwm_.get_load(loadid);
                    // if the old and new loads are different, spscial logic needed
                    bool changed_active = false;
                    if (d.HasMember("activeFlag")) {
                        bool activeFlag = d["activeFlag"].GetBool();
                        if (p_cust->active_flag_ != activeFlag) {
                           changed_active = true;
                           p_cust->active_flag_ = activeFlag;
                        }
                    } else {
                        if (p_cust->active_flag_) {
                           changed_active = true;
                           p_cust->active_flag_ = false;
                        }
                    }

                    if (changed_active) {
                       if (p_cust->active_flag_)
                          nwm_.add_customer(id, p_newload->nid_);
                       else
                          nwm_.remove_customer(id, p_cust->p_load_->nid_);
                    }

                    if (p_newload != p_cust->p_load_) {
                       if (p_cust->p_load_) {
                          if (p_newload) {
                             nwm_.transfer_customer(id, p_newload->nid_, p_cust->p_load_->nid_);
                          } else { // no new load, removing
                             nwm_.remove_customer(id, p_cust->p_load_->nid_);
                          }
                       } else { // no existing load
                          nwm_.add_customer(id, p_newload->nid_);
                       }
                    }

                    // TODO: update other attributes
                   
                    bool changed_priority = false;
                    if (d.HasMember("priorityEnum")) {
                       string priorityVal = d["priorityEnum"].GetString();
                       if (!p_cust->is_priority()) {
                          // actual value does not matter, bleah
                          p_cust->priority_id_ = 1;
                          changed_priority = true;
                       }
                    } else if (p_cust->is_priority()) {
                       changed_priority = true;
                       p_cust->priority_id_ = 0;
                    }

                    if (changed_priority) {
                       if (p_cust->p_currjob_) {
                          nwm_.add_to_dirty_outages(p_cust->p_currjob_);
                       }
                    }

                    bool changed_critical = false;
                    if (d.HasMember("criticalEnum")) {
                       string criticalVal = d["criticalEnum"].GetString();
                       if (!p_cust->is_critical()) {
                          // actual value does not matter, bleah
                          // should be using a boolean
                          p_cust->critical_id_ = 1;
                          changed_critical = true;
                       }
                    } else if (p_cust->is_critical()) {
                       changed_critical = true;
                       p_cust->critical_id_ = 0;
                    }

                    if (changed_critical) {
                       if (p_cust->p_currjob_) {
                          nwm_.add_to_dirty_outages(p_cust->p_currjob_);
                       }
                    }
                }
                catch(const std::exception& se)
                {
                    cout << "Error [" << se.what() << "] parsing customer update JSON: " << endl << event.str_JSON_ << endl;
                    return;
                }
            }
        } else if (event.str_model_ == "StormMode")
        {
        } else if (event.str_model_ == "CrewAssign")
        {
            if (event.str_event_type_ == "add")
            {
                // TODO test before committing
                /*
                JSONJobCrewAssignment jjca;
			    jjca.p_jca_ = new JobCrewAssignment;
                if (jjca.from_JSON(event.str_JSON_))
                {
                    Job* p_job = nwm_.get_job(jjca.job_id_);
                    if (p_job)
                    {
                        p_job->assignments_.push_unsorted(jjca.p_jca_);

                    } else
                    {
                        stringstream ss;
                        ss << "Job " << jjca.job_id_ << " for crew assignment " << jjca.p_jca_->id_ << " was not found...";
                        log(LV_ERROR,ss.str(),true);

                        delete jjca.p_jca_;
                    }

    	        } else
                {
                    cout << "Job crew assignment " << jjca.p_jca_->id_ << " extraction error..." << endl;
                    delete jjca.p_jca_;
                }
                */

            } else if (event.str_event_type_ == "update")
            {
                try
                {
                    /*
                    rapidjson::Document d;
                    d.Parse<0>(event.str_JSON_.c_str());

                    // NOTE that the id is provided in the event header, not in the body JSON.
                    // int_fast64_t id = d["id"].GetInt64();
                    int_fast64_t id = event.id_;
                    */

                    // TODO
                    /*
    			    JobCrewAssignment* p_jca = find... in... p_job->assignments_
                    if (p_jca == 0)
                    {
                        cout << "Error finding customer contact " << id << " to update..." << endl;
                        return;
                    }

                    if (d.HasMember("jobId")) 
                    {                        
                        int_fast64_t jobId = d["jobId"].GetInt64();
                        if (jobId != p_contact->jobId_)
                            nwm_.update_...
                    } 

                    cout << "Received update for job crew assignment [" << p_jca->id_ << "]..." << endl;
                    */

                }
                catch(const std::exception& se)
                {
                    cout << "Error [" << se.what() << "] parsing crew assignment update JSON: " << endl << event.str_JSON_ << endl;
                    return;
                }
            }

        } else if (event.str_model_ == "JobAction")
        {
        } else if (event.str_model_ == "Contact")
        {
            if (event.str_event_type_ == "add")
            {
                Contact contact;
                if (contact.from_JSON(event.str_JSON_))
                {
                    stringstream ss;
                    ss << "Received new contact [" << contact.id_ << "]...";
                    log(LV_INFO,ss.str());
                    
                    // DEBUG log the whole contact for now, HEAVY
                    log(LV_INFO,event.str_JSON_,true);

                    if (!contact.electricFlag_ || !contact.outageFlag_ || contact.plannedFlag_)
                    {
                        stringstream ss;
                        ss << "Received non unplanned electric outage contact, ignoring: " << contact.electricFlag_ << " o: " << contact.outageFlag_ << " p: " << contact.plannedFlag_;
                        log(LV_INFO,ss.str());
                        return;
                    }

                    handle_new_contact(contact);
                }

            } else if (event.str_event_type_ == "update")
            {
                // Updates may have 0...n field updates (any field except id).
                // Since OA does not necessarily use all fields, the used ones may not have any changes.
                // We have to step through and see what we got!

                try
                {
                    rapidjson::Document d;
                    d.Parse<0>(event.str_JSON_.c_str());

                    // NOTE that the id is provided in the event header, not in the body JSON.
                    // int_fast64_t id = d["id"].GetInt64();
                    int_fast64_t id = event.id_;

                    Contact* p_contact = nwm_.get_contact(id);
                    if (p_contact == 0)
                    {
                        cout << "Error finding customer contact " << id << " to update..." << endl;
                        return;
                    }

                    if (d.HasMember("jobId")) 
                    {                        
                        int_fast64_t jobId = d["jobId"].GetInt64();
                        if (jobId != p_contact->jobId_)
                            nwm_.update_customer_contact(*p_contact,jobId);
                    } 

/*  use in customer chunking
                        if (data.HasMember("networkModelUuid"))
                        {
                            // TODO repredict on change?

		                    const rapidjson::Value& val_nid = data["networkModelUuid"];
		                    for (auto itn = val_nid.Begin(); itn != val_nid.End(); ++itn)
		                    {
			                    p_contact->networkObjectId_.id_[itn - val_nid.Begin()] = itn->GetInt64();
		                    }
                        }
*/


                    // TODO: check for cancellation
                    //
                    if (d.HasMember("cancelledFlag")) {
                        bool cancelled = d["cancelledFlag"].GetBool();
                        if (cancelled) {
                           cout << "Got cancel for contact: " << p_contact->id_ << endl;
                            //mark island for reprediction
                            Load* p_load = nwm_.get_load(p_contact->networkObjectId_);
                            if (!p_load) {
                                cout << "Got cancel for contact: " << p_contact->id_ << " but no associated load" << endl;
                            } else {
                                 mark_island_for_analysis(p_load->island_[EN_CURR]);
                            }
                            // remove contact
                            Customer* p_customer = nwm_.get_customer(p_contact->customerId_);
                            if (p_customer == 0)
                            {
                               cout << "cancelling contact, but no customer found" << endl;
                            } else {
                               nwm_.remove_customer_contact(p_customer);
                            }

                        }
                    } 
                    if (d.HasMember("hazardConditionEnums")) 
                    {
                        int old_cnt = p_contact->hazard_cnt_;
                        p_contact->hazard_cnt_ = 1;

                        if (old_cnt == 0)
                            nwm_.add_to_dirty_outages(p_contact->jobId_);

                    } else if (p_contact->is_hazard())
                    {
                        p_contact->hazard_cnt_ = 0;
                        nwm_.add_to_dirty_outages(p_contact->jobId_);
                    }

                        // Not handled at this time...
                        // if (data.HasMember("customerId"))
                        // if (data.HasMember("workAreaId"))

                    cout << "Received update for contact [" << p_contact->id_ << "]..." << endl;

                }
                catch(const std::exception& se)
                {
                    cout << "Error [" << se.what() << "] parsing contact update JSON: " << endl << event.str_JSON_ << endl;
                    return;
                }
            }
        } // if contact
    }
    catch(const std::exception& se)
    {
        cout << "======" << endl << "Error [" << se.what() << "] parsing chunked event: " << endl << event.str_JSON_ << "======" << endl << endl;
    }
}


