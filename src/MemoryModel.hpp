/*
+ * MemoryModel.hpp
 *
 *  Created on: Apr 26, 2015
 *      Author: m
 */

#ifndef MEMORY_MODEL_HPP_
#define MEMORY_MODEL_HPP_

#include <utilities.hpp>            // So we can set the value of the IP_LOGGING_LEVEL default pref
#include "MyApplicationModel.hpp"	// We break out the app model definition to keep it clean; we still manage the actual objects in this module


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------

// Note that we want GLOBAL ACCESS to a GENERIC local model.
class MemoryModel;                  // predefined here; class is defined below
extern MemoryModel* g_p_local;


// NOTE: see wiki for C++11 scoped enumeration (and why we haven't used it here yet).  Old skool is still cool:
typedef enum
{
    // assert( SP_COUNT == 1 );
    // Keep synched with init_str_pref_config()
    // The order doesn't matter as long as the code is recompiled.
    SP_FIRST = 0                        ,
    SP_DATABASE_DESCRIPTION             = SP_FIRST,
    // SP_TODO_STRING_PREF2             ,

    SP_COUNT
} STRING_PREF_INDEX;

typedef enum
{
    // assert( IP_COUNT == 7 );
    // Keep synched with init_int_pref_config()
    IP_FIRST = 0                         ,
    IP_DB_VERSION = IP_FIRST             ,
    IP_PROFILING_LEVEL                   ,
    IP_IGNORE_SSL_ERRORS                 ,
    IP_LOGGING_LEVEL                     ,
    IP_STARTUP_DELAY_MS                  ,
    IP_MAIN_LOOP_RATE_MS                 ,
    IP_RUN_ANALYSIS_FREQUENCY_MS         ,

    IP_COUNT
} INT_PREF_INDEX;


// ------------
// PREF HELPERS
// ------------
// These structures give user names to prefs, and provide default values.
class StringPref
{
public:
    StringPref( int_fast32_t nIndex, string strName, string strDefaultValue )
    :
        // Init vars.
        nIndex_( nIndex ),
        strName_( strName ),
        strDefaultValue_( strDefaultValue )
    {}
    int_fast32_t nIndex_;
    string strName_;
    string strDefaultValue_;
};
class IntPref
{
public:
    IntPref( int_fast32_t nIndex, string strName, int_fast32_t nDefaultValue )
    :
        // Init vars.
        nIndex_( nIndex ),
        strName_( strName ),
        nDefaultValue_( nDefaultValue )
    {}
    int_fast32_t nIndex_;
    string strName_;
    int_fast32_t nDefaultValue_;
};
static const std::vector<StringPref> init_str_pref_config()
{
    assert( SP_COUNT == 1 );
    std::vector<StringPref> v;
    v.push_back(StringPref( SP_DATABASE_DESCRIPTION             , "DatabaseDescription"        , "** YOU SHOULD ADD A DatabaseDescription PREF **" ));

    return v;
}
static const std::vector<StringPref> s_str_pref_config = init_str_pref_config();
static const std::vector<IntPref> init_int_pref_config()
{
    assert( IP_COUNT == 7 );
    std::vector<IntPref> v;
    v.push_back(IntPref( IP_DB_VERSION                        , "DatabaseVersion"                 , 1         ));     // The upgrade process should keep this synced with Version.h
    v.push_back(IntPref( IP_PROFILING_LEVEL                   , "ProfilingLevel"                  , 0         ));     // Set to turn on different levels of profiling output
    v.push_back(IntPref( IP_IGNORE_SSL_ERRORS                 , "IgnoreSSLErrors"                 , 1         ));     // Default to ignore to make it WORK; more security-aware folks can then turn it on
    v.push_back(IntPref( IP_LOGGING_LEVEL                     , "LoggingLevel"                    , LV_DEBUG  ));     // We'll go with DEBUG while under development, reduce later!
    v.push_back(IntPref( IP_STARTUP_DELAY_MS                  , "StartupDelayMilliseconds"        , 50        ));     // CAREFUL with this one!
    v.push_back(IntPref( IP_MAIN_LOOP_RATE_MS                 , "MainLoopRateMilliseconds"        , 100       ));     // CAREFUL with this one!
    v.push_back(IntPref( IP_RUN_ANALYSIS_FREQUENCY_MS         , "RunAnalysisFreqMilliseconds"     , 5000      ));     // May turn out to be strategy-dependent; we'll go fast while under dev, revert to >3 seconds in production??  We shall see...

    return v;
}
static const std::vector<IntPref> s_int_pref_config = init_int_pref_config();
// ------------

// ------------------------------------------------------------------------------


// Our in-memory and persist-to-disk local model datastore.
class MemoryModel
{
public:

    MemoryModel();

    virtual ~MemoryModel();

    virtual bool initialize(bool bTest);

    const std::string& get_pref( STRING_PREF_INDEX sp );
    int_fast32_t get_pref( INT_PREF_INDEX ip );
    std::string get_pref_as_string( INT_PREF_INDEX ip );

    virtual void set_pref( STRING_PREF_INDEX sp, const std::string& str_new_value );
    virtual void set_pref( INT_PREF_INDEX ip, const int_fast32_t n_new_value );

    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
    // Manage contained objects.
    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
    AppUser& addAppUser(AppUser* p_au);
    AppUser& addAppUserToMemory(AppUser* p_au);
    AppUserIt findAppUser(int_fast32_t n_id);

    // Database functions.
    virtual bool writeTire(Tire& t) = 0;
    virtual bool writeCar(Car& c) = 0;
    virtual bool writeAppUser(AppUser& au) = 0;

    virtual bool deleteTire(Tire& t) = 0;
    virtual bool deleteCar(Car& c) = 0;
    virtual bool deleteAppUser(AppUser& au) = 0;

    // TODO as needed
    // virtual bool backup(const string& prefix, const string& suffix) = 0;
    // virtual bool performAfterHoursMaintenance(const ptime& pt) = 0;

    bool saveDirtyObjectsAsNeeded(bool bSkipQuotes = false);
    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

    // NOTE: These run from http thread.
    // virtual string buildHistory(const string& strUsername, const string& strAccountId) = 0;

    // Transactions are a Very Good Thing.
    // Pair up the start and end in the same thread.
    virtual bool startTransaction(bool bWritable = true, bool bCreatable = false) = 0;
    virtual bool endTransaction() = 0;
    virtual bool bTransactionComplete() = 0;

    AppUsers users_;

    /* if needed...
    // ----------------------------------------------------------
    // Reference-counted smart references into a collection.
    // ----------------------------------------------------------
    // Every new parent object with a reference requests one of these, and releases when deleted.
    // If the requested object is not in the collection, add it.
    // Once refcount goes back to zero, remove object from collection.
    SQ& addSQSmartRef(const string& str_name);
    SQIt getSQ(const string& str_name);
    void removeSQSmartRef(const string& str_name);
    void removeSQSmartRef(SQIt itsq);
    SQs sq_;
    // ----------------------------------------------------------
    */

protected:

    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
    // Manage contained objects.
    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
    virtual bool readTires() = 0;
    virtual bool readCars() = 0;
    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

    // These implementations are done in derived class.
    virtual bool create_new_db() = 0;
    virtual bool upgrade_db_as_needed() = 0;
    virtual bool read_prefs() = 0;
    virtual bool write_pref(STRING_PREF_INDEX sp) = 0;
    virtual bool write_pref(INT_PREF_INDEX ip) = 0;
    virtual bool initializeMaxDbIds() = 0;

    bool write_config();

    void clearUsers();

    // Actual in-memory preferences.
    std::vector<std::string> str_prefs_;
    std::vector<int_fast32_t> int_prefs_;
};


#endif // MEMORY_MODEL_HPP_
