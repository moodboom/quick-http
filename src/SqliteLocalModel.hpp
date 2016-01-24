/*
+ * SqliteLocalModel.hpp
 *
 *  Created on: Apr 26, 2015
 *      Author: m
 */

#ifndef SQLITE_LOCAL_MODEL_HPP_
#define SQLITE_LOCAL_MODEL_HPP_

#include <mutex>
#include <sqlite/SQLiteCpp/SQLiteCpp.h>     // Our C++ wrapper for our sqlite C code; we grabbed static versions of both and compile them right into our app
#include "MemoryModel.hpp"


class SqliteLocalModel : public MemoryModel
{
    typedef MemoryModel inherited;

public:
    SqliteLocalModel();
    virtual ~SqliteLocalModel();

    virtual bool initialize(bool bTest);

    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
    // Manage contained objects.
    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
    virtual bool writeTire(Tire& t);
    virtual bool writeCar(Car& c);
    virtual bool writeAppUser(AppUser& au);

    virtual bool deleteTire(Tire& t);
    virtual bool deleteCar(Car& c);
    virtual bool deleteAppUser(AppUser& au);

    // TODO as needed
    virtual bool backup(const string& prefix, const string& suffix);
    virtual bool performAfterHoursMaintenance(const ptime& pt);
    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

    // TODO
    // NOTE: These run from http thread.
    // virtual string buildModelHistory(int_fast32_t model_id);

    // Transactions are a Very Good Thing.
    // Here, we implement refcount so you can embed start/end pairs.
    // But of course you must still make sure they match up.
    // Pair up the start and end in the same thread.
    virtual bool startTransaction(bool bWritable = true, bool bCreatable = false);
    virtual bool endTransaction();
    virtual bool bTransactionComplete() { return transaction_refcount_ == 0; }

    bool bTest() { return b_test_; }

protected:

    // Implementation is done in derived class.
    virtual bool create_new_db();
    virtual bool upgrade_db_as_needed();
    virtual bool read_prefs();
    virtual bool write_pref(STRING_PREF_INDEX sp);
    virtual bool write_pref(INT_PREF_INDEX ip);
    virtual bool initializeMaxDbIds();

    virtual bool readAppUsers();
    virtual bool readCars();
    virtual bool readTires();

    // Helpers
    string getCreateSQL(const string& table, const vector<pair<string,string>>& cols);
    string getColumnList(const vector<pair<string,string>>& cols);
    string getSelectSQL(const string& table, const vector<pair<string,string>>& cols);
    string getInsertSQLStart(const string& table, const vector<pair<string,string>>& cols, bool bOrReplace = false);
    int migrateTable(const string& table, const vector<pair<string,string>>& cols);

    // Transactions are a Very Good Thing.
    SQLite::Database* pdb_;
    SQLite::Transaction* ptr_;
    int_fast32_t transaction_refcount_;
    std::mutex transaction_mutex_;                  // We do our own work to keep our db access threadsafe
    // boost::shared_mutex transaction_guard_;      // TODO consider sqlite thread-safe features for simultaneous multithreaded read access
    bool b_trans_creatable_;
    bool b_trans_writable_;

    bool b_test_;
};


#endif /* SQLITE_LOCAL_MODEL_HPP_ */
