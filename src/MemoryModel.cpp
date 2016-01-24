/*

 Copyright (c) A better Software http://abettersoftware.com
 All rights reserved.

*/

#include "MemoryModel.hpp"
#include "Version.hpp"              // For c_nDatabaseVersion


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------

// We want GLOBAL ACCESS to a dynamically-created GENERIC local model.
// This allows:
//      1) fast easy development
//      2) easy replacement of implementation
MemoryModel* g_p_local = 0;

// ------------------------------------------------------------------------------


MemoryModel::MemoryModel()
:
    // Init vars.
    str_prefs_(SP_COUNT),
    int_prefs_(IP_COUNT)

    // TODO use persistent key objects to find in unordered_set.
    // HOWEVER this is difficult - you really want to define the set to have constant keys, obviously.
    // But if you want a key object, you need to have a way to force its keys to change.
    // note that getEtradeBroker() can't be called, it's not ready yet
    // ba_key_(getEtradeBroker()),
    // as_key_(ba_key_,Stock("_unspecified"))
{
}

MemoryModel::~MemoryModel()
{
    clearUsers();

    // TODO clean up ALL container allocations

}


void MemoryModel::clearUsers()
{
    for (auto pu : users_)
        delete pu;
    users_.clear();
}


bool MemoryModel::initialize(bool bTest)
{
    // Read in all the prefs on startup, then we can just get them from memory later as needed.

    // First, we set all prefs to default values.
    for (int i = 0; i < SP_COUNT; ++i)
        str_prefs_[i] = s_str_pref_config[i].strDefaultValue_;
    for (int i = 0; i < IP_COUNT; ++i)
        int_prefs_[i] = s_int_pref_config[i].nDefaultValue_;

    // Now load the prefs tables, which only contain values that have been saved.
    // This includes the version number, which lets us do any needed upgraded.
    // If the pref load fails, we assume that we must create an entire new database, backing up any we find.
    startTransaction();
    try
    {
        if (read_prefs())
        {
            // We can now switch to the logging level requested in the prefs.
            g_current_log_verbosity = (LOG_TO_FILE_VERBOSITY)get_pref(IP_LOGGING_LEVEL);

            if (!upgrade_db_as_needed())
            {
                endTransaction();
                return false;
            }

            log(LV_ALWAYS,get_pref(SP_DATABASE_DESCRIPTION));

        } else
        {
            // NOTE: If we don't find an existing database,
            // we NEED TO EXIT so the user can enter basic config data.
            // In particular, we can't start without an E*Trade developer key.
            create_new_db();

            log(LV_ALWAYS,"");
            log(LV_ALWAYS,
                "********************** NOTE **********************                             \n"
                "A new database has been created.                                               \n"
                "You should configure it with basic startup data, then restart.                 \n"
                "See the database for details.                                                  \n"
                "                                                                               \n"
                " SETTING UP A NEW DATABASE                                                     \n"
                " =========================                                                     \n"
                "    TODO                                                                       \n"
                "********************** NOTE **********************                             \n"
            );
            log(LV_ALWAYS,"");

            endTransaction();
            return false;
        }

        // Extract and set all the max db_ids, so we can track them ourselves in memory.
        initializeMaxDbIds();

        // Load in the correct order.
        if (!readCars()  ) return false;
        if (!readTires()   	) return false;

    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR model initialize: " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    endTransaction();
    return true;
}


bool MemoryModel::write_config()
{
    startTransaction();

    // We have just created a new database.
    // Set and write the version.
    set_pref(IP_DB_VERSION,c_nDatabaseVersion);

    // Carry over or initialize the basic config settings.

    // eg:
    // write_pref(SP_CONSUMER_KEY);

    // Write some defaults so the user can override if desired.
    // These should be carried over on database replacements,
    // so hopefully we can preserve important config settings.
    write_pref(SP_DATABASE_DESCRIPTION);
    write_pref(IP_RUN_ANALYSIS_FREQUENCY_MS);
    write_pref(IP_LOGGING_LEVEL);
    write_pref(IP_STARTUP_DELAY_MS);
    write_pref(IP_MAIN_LOOP_RATE_MS);

    endTransaction();

    return true;
}


const string& MemoryModel::get_pref( STRING_PREF_INDEX sp )
{
    return str_prefs_[sp];
}

int_fast32_t MemoryModel::get_pref( INT_PREF_INDEX ip )
{
    return int_prefs_[ip];
}

string MemoryModel::get_pref_as_string( INT_PREF_INDEX ip )
{
    stringstream ss_int;
    ss_int << int_prefs_[ip];
    return ss_int.str();
}

void MemoryModel::set_pref( STRING_PREF_INDEX sp, const string& str_new_value )
{
    str_prefs_[sp] = str_new_value;
    write_pref(sp);
}

void MemoryModel::set_pref( INT_PREF_INDEX ip, const int_fast32_t n_new_value )
{
    int_prefs_[ip] = n_new_value;
    write_pref(ip);
}


AppUser& MemoryModel::addAppUser(AppUser* p_au)
{
    if (!writeAppUser(*p_au))
        log(LV_ERROR,"ERROR writing APS!");

    return addAppUserToMemory(p_au);
}
AppUser& MemoryModel::addAppUserToMemory(AppUser* p_au)
{
    // We need a valid db id.
    assert(p_au->db_id_ != -1);

    users_.insert(p_au);
    return *p_au;
}
AppUsers::iterator MemoryModel::findAppUser(int_fast32_t n_id)
{
    g_AUKey.db_id_ = n_id;
    return users_.find(&g_AUKey);
}


bool MemoryModel::saveDirtyObjectsAsNeeded(bool bSkipQuotes)
{
    // Loop through all objects to see if we NEED to save.
    // If not, we won't even start a transaction, saving us from hitting the db at all.  Well worth it.
    bool bNeeded = false;

    // TODO make "Deep" dirty and write calls so they take care of their own children
    // then we just do the users here
    for (auto pau : users_)
    {
        if (pau->bActive() && pau->bDirty())
        {
            bNeeded = true;
            break;
        }
        if (!bNeeded) for (auto pcar : pau->cars_)
        {
            if (pcar->bActive() && pcar->bDirty())
            {
                bNeeded = true;
                break;
            }
            if (!bNeeded) for (auto pd : pcar->tires_)
            {
                if (pd->bDirty())
                {
                    bNeeded = true;
                    break;
                }
            }
        }
    }

    if (bNeeded)
    {
        startTransaction();
        for (auto pau : users_)
        {
            if (pau->bActive() && pau->bDirty())
                writeAppUser(*pau);

            for (auto pcar : pau->cars_)
            {
                if (pcar->bActive() && pcar->bDirty())
                    writeCar(*pcar);

                for (auto pd : pcar->tires_)
                {
                    if (pd->bDirty())
                        writeTire(*pd);
                }
            }
        }
        endTransaction();
    }
}
