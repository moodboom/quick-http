/*

 Copyright (c) A better Software http://abettersoftware.com

*/

#include <utilities.hpp>            // For LOG_TO_FILE_VERBOSITY
#include "SqliteLocalModel.hpp"
#include "Version.hpp"              // For c_nDatabaseVersion


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------

// NOTE see Version.h for database versioning.

string s_str_db_name;
const string c_str_db_name_prod = "my_quick_http_app.sqlite";
const string c_str_db_name_test = "my_quick_http_app_test.sqlite";

const string c_str_Tablename_PrefInt    = "PrefInt";
const string c_str_Tablename_PrefStr    = "PrefStr";
const string c_str_Tablename_Tires      = "Tires";
const string c_str_Tablename_Cars       = "Cars";
const string c_str_Tablename_AppUsers   = "AppUsers";

// We have often had to ALTER tables to add new columns.
// There is NO WAY to reorder columns via ALTER in sqlite.
// An exact column order is REQUIRED during loading when using [SELECT * ...], [INSERT ... VALUES(...)].
// So the best solution is to specify requested columns in the SQL, not rely on *, etc.
// We set them up here, then use them in CREATE, SELECT and INSERT SQL, yay!
// NOTE that we cannot rename or drop columns without recreating a table.  Sucks!

const vector<pair<string,string>> cv_cols_PrefInt                   =
{
    {"name" ,"TEXT PRIMARY KEY NOT NULL,"},
    {"value","INTEGER"                   }
};
const vector<pair<string,string>> cv_cols_PrefStr                   =
{
    {"name" ,"TEXT PRIMARY KEY NOT NULL,"},
    {"value","TEXT"                      }
};
const vector<pair<string,string>> cv_cols_Tires                     =
{
    {"id"        ,"INTEGER PRIMARY KEY NOT NULL,"},
    {"appuser_id","INTEGER NOT NULL,"            },
    {"car_id"    ,"INTEGER NOT NULL,"            },
    {"name"      ,"TEXT,"                        },
    {"type"      ,"TEXT"                         }
};
const vector<pair<string,string>> cv_cols_Cars    =
{
    {"id"           ,"INTEGER PRIMARY KEY NOT NULL,"},
    {"appuser_id"   ,"INTEGER NOT NULL,"            },
    {"name"         ,"TEXT"                         }
};
const vector<pair<string,string>> cv_cols_AppUsers                  =
{
    {"id"                           ,"INTEGER PRIMARY KEY NOT NULL,"},
    {"username"                     ,"TEXT,"                        },
    {"password"                     ,"TEXT,"                        },
    {"b_active"                     ,"INTEGER"                      }
};
// ------------------------------------------------------------------------------


SqliteLocalModel::SqliteLocalModel()
:
    // init vars
    pdb_(0),
    ptr_(0),
    transaction_refcount_(0)
{}


SqliteLocalModel::~SqliteLocalModel()
{
}


bool SqliteLocalModel::initialize(bool bTest)
{
    // Use test as minimally as possible.
    // In this class, if we can't upgrade an incompatible db, we need to quit if not test, as the user needs to set it up.
    b_test_ = bTest;

    if (b_test_)
        s_str_db_name = c_str_db_name_test;
    else
        s_str_db_name = c_str_db_name_prod;

    // For sqlite, we open and close a local file on every read or write, rather than keep a file open.
    // Here, we make sure the db exists, before letting the base class do all the initializing.
    try
    {
        // Try to open the db, creating it as needed.
        SQLite::Database db(s_str_db_name, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
    }
    catch (std::exception& e)
    {
        // Try one more time, backing out any problem file first.
        try
        {
            log(LV_WARNING,"WARNING: can't access existing sqlite db, attempting to recreate...");
            archive_any_old_file(s_str_db_name,"",string("__failed") + generate_uuid() + ".sqlite");
            SQLite::Database dbtry2(s_str_db_name, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
        }
        catch (std::exception& e)
        {
            g_ss.str(std::string());
            g_ss << "ERROR SQLite initialize: " << e.what();
            log(LV_ERROR,g_ss.str());
            return false;
        }
    }

    return inherited::initialize(bTest);
}


// Transactions are a Very Good Thing.
// Here, we implement refcount so you can embed start/end pairs.
// But of course you must still make sure they match up.
// Pair up the start and end in the same thread.
bool SqliteLocalModel::startTransaction(bool bWritable, bool bCreatable)
{
    ++transaction_refcount_;
    if (transaction_refcount_ == 1)
    {
        b_trans_writable_ = bWritable;
        b_trans_creatable_ = bCreatable;
        try
        {
            // DEBUG
            // cout << "LOCKING SQLITE..." << endl;

            // We need MUTEXED DATABASE ACCESS HERE
            // as this will be in its own thread
            // competing with main thread for access.
            // Even though this is a read-only access, we don't yet have a handle on allowing that
            // in SQLiteCPP in multiple threads.
            transaction_mutex_.lock();

            // TODO revisit, i'm pretty sure sqlite can do multithreaded read access.
            // BUT it might not be worth it and better to just make the access exclusive ourselves.
            // I've heard old versions of sqlite can timeout after 5 second blocks which can happen.  Yuck.
            // http://beets.radbox.org/blog/sqlite-nightmare.html
            // boost::upgrade_lock<boost::shared_mutex>* p lock_(transaction_guard_);
            // boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

            assert(ptr_ == 0);

            pdb_ = new SQLite::Database(
                s_str_db_name,
                bCreatable? (SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE) : (bWritable? SQLITE_OPEN_READWRITE : SQLITE_OPEN_READONLY)
            );
            ptr_ = new SQLite::Transaction(*pdb_);
        }
        catch (std::exception& e)
        {
            g_ss.str(std::string());
            g_ss << "ERROR startTransaction(): " << e.what();
            log(LV_ERROR,g_ss.str());
            return false;
        }
    } else
    {
        // Check to see if we need a transaction type upgrade.
        if (bWritable && !b_trans_writable_ || bCreatable && !b_trans_creatable_)
        {
            // Upgrade time!  Cache the refcount, fake a new top-level transaction with the new type, then reset the count.  Slick dangerous semi-recursive shit.
            int_fast32_t temp_refcount = transaction_refcount_;
            transaction_refcount_ = 1;
            endTransaction();
            startTransaction(bWritable,bCreatable);
            transaction_refcount_ = temp_refcount;
        }
    }

    // DEBUG there are times when we'll miss a pair and need to debug :-(
    // cout << "+=" << transaction_refcount_ << endl;

    return true;
}
bool SqliteLocalModel::endTransaction()
{
    // Make sure you don't call endTransaction() without a matching startTransaction pair.
    assert(transaction_refcount_ > 0);

    --transaction_refcount_;

    if (transaction_refcount_ == 0)
    {
        try
        {
            // DEBUG
            // cout << "WRITING SQLITE...";

            ptr_->commit();
            delete ptr_;
            delete pdb_;
            ptr_ = 0;
            pdb_ = 0;

            transaction_mutex_.unlock();

            // DEBUG
            // cout << "DONE" << endl;
        }
        catch (std::exception& e)
        {
            g_ss.str(std::string());
            g_ss << "ERROR endTransaction(): " << e.what();
            log(LV_ERROR,g_ss.str());
            return false;
        }
    }

    // DEBUG there are times when we'll miss a pair and need to debug :-(
    // cout << "-=" << transaction_refcount_ << endl;

    return true;
}


bool SqliteLocalModel::create_new_db()
{
    int nb = -99;
    archive_any_old_file(s_str_db_name,"",string("__v")+boost::lexical_cast<std::string>(get_pref(IP_DB_VERSION))+"__" + generate_uuid() + ".sqlite");
    startTransaction(true,true);

    try
    {
        SQLite::Database& db = *pdb_;
        g_ss.str(std::string());
        g_ss << "Creating new sqlite database [v" << c_nDatabaseVersion << "]";
        log(LV_INFO,g_ss.str());

        nb = db.exec(getCreateSQL(c_str_Tablename_PrefInt               ,cv_cols_PrefInt                ));
        nb = db.exec(getCreateSQL(c_str_Tablename_PrefStr               ,cv_cols_PrefStr                ));
        nb = db.exec(getCreateSQL(c_str_Tablename_Tires                 ,cv_cols_Tires                  ));
        nb = db.exec(getCreateSQL(c_str_Tablename_Cars                  ,cv_cols_Cars                   ));
        nb = db.exec(getCreateSQL(c_str_Tablename_AppUsers              ,cv_cols_AppUsers               ));

        // Put new tables here...

        // Inject basic config data.
        // If this is a new database due to compatibility change, hopefully config was loaded successfully from previous db.
        // If not they at least get defaults which can be updated as needed.
        write_config();
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR SQLite create_new_db exec result[" << nb << "]: " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    endTransaction();
    return true;
}


// TODO there's lots of logic in here that we should abstract up to base class
bool SqliteLocalModel::upgrade_db_as_needed()
{
    // Compare version from db with version from code... most of the time we will match and return.  :-)
    auto db_orig_version = get_pref(IP_DB_VERSION);
    if (db_orig_version == c_nDatabaseVersion)
    {
        g_ss.str(std::string());
        g_ss << "Loading " << s_str_db_name << " [v" << c_nDatabaseVersion << "]";
        log(LV_INFO,g_ss.str());
        return true;
    }

    if (db_orig_version < c_nLastCompatibleDBVersion)
    {
        if (b_test_)
        {
            log(LV_WARNING,"WARNING: Your SQLite db was too old.");
            log(LV_WARNING,"It is being archived and recreated from scratch.");
            log(LV_WARNING,"Basic config will be carried forward as possible,");
            log(LV_WARNING,"and new test data will be injected.");
            return create_new_db();

        } else
        {
            create_new_db();
            log(LV_ALWAYS,"");
            log(LV_ALWAYS,
                "********************** NOTE **********************                             \n"
                "ERROR: Your production db was too old,                                         \n"
                "so a new up-to-date database has been created.                                 \n"
                "Basic startup data was carried over,                                           \n"
                "but you will need to migrate users, accounts, etc.                             \n"
                "                                                                               \n"
                " SETTING UP A NEW DATABASE                                                     \n"
                " =========================                                                     \n"
                "    start the app, let it upgrade from incompatible db and quit                \n"
                "    update the database with basic starting required data,                     \n"
                "    SAVE and CLOSE db                                                          \n"
                "    restart the app                                                            \n"
                "    as needed: quit the app, adjust data, restart                              \n"
                "                                                                               \n"
                "********************** NOTE **********************                             \n"
            );
            return false;
        }
    }

    int nb = -199;
    startTransaction();
    try
    {
        SQLite::Database& db = *pdb_;

        // UPGRADE LIMITATIONS
        //
        // Add table: ok
        //
        // Add column: ok
        // NOTE: We have often had to ALTER tables to add new columns.
        // There is no way to reorder columns via ALTER in sqlite.
        // An exact column order is required during loading when using [SELECT * ...], [INSERT ... VALUES(...)].
        // So don't use those.
        // In this code, we have set up a specific list of columns, and we use that in our CREATE, SELECT and INSERT SQL.
        // So feel free to ALTER tables to add new non-keyed columns at the end!
        //
        // Rename col, drop col, change key: ok
        // NOTE that in sqlite we cannot rename or drop columns without recreating a table.
        // So for this one, you have to alter the table as needed, then use migrate_table(tablename).
        // That will create a new temp table, copy over all data, and then remove old and rename new.
        // Cool!

        /*
        if (db_orig_version < ...)
        {
        }


        // OLD examples

        if (db_orig_version < 1)
        {
            // Newly added pref, stuff it so user can update it.
            write_pref(SP_DATABASE_DESCRIPTION);
        }
        if (db_orig_version < 2)
        {
            backup_any_old_file(s_str_db_name,string("__v")+boost::lexical_cast<std::string>(db_orig_version)+"__" + generate_uuid() + ".sqlite");

            g_ss.str(std::string());
            g_ss << "ALTER TABLE "  << c_str_Tablename_AutotradedStocks << " ADD COLUMN b_initialized INTEGER;"
                 << "UPDATE " << c_str_Tablename_AutotradedStocks << " SET b_initialized = 1";
            nb = db.exec(g_ss.str());

            // We need to add new field BA d_minimum_purchase_size, and rename autotraded_value_change to available_cash.
            // To rename, we need to create a new column, copy over data to it, then migrate the table.

            g_ss.str(std::string());
            g_ss << "ALTER TABLE "  << c_str_Tablename_Accounts << " ADD COLUMN minimum_purchase_size REAL;"
                 << "UPDATE " << c_str_Tablename_Accounts << " SET minimum_purchase_size = 3000.00";
            nb = db.exec(g_ss.str());

            g_ss.str(std::string());
            g_ss << "ALTER TABLE "  << c_str_Tablename_Accounts << " ADD COLUMN available_cash REAL;"
                 << "UPDATE " << c_str_Tablename_Accounts << " SET available_cash = autotraded_value_change";
            nb = db.exec(g_ss.str());
            nb = migrateTable(c_str_Tablename_Accounts, cv_cols_Accounts);

            g_ss.str(std::string());
            g_ss << "ALTER TABLE "  << c_str_Tablename_AccountHistory << " ADD COLUMN available_cash REAL;"
                 << "UPDATE " << c_str_Tablename_AccountHistory << " SET available_cash = autotraded_value_change";
            nb = db.exec(g_ss.str());
            nb = migrateTable(c_str_Tablename_AccountHistory, cv_cols_AccountHistory);
        }

        // THIS IS SAFE if you just need to add a new non-keyed column...
        if (db_orig_version < 3)
        {
            g_ss.str(std::string());
            g_ss << "ALTER TABLE " << c_str_Tablename_StockPicks << " ADD COLUMN b_initialized INTEGER";
            nb = db.exec(g_ss.str());
        }


        // If a new pref was added that you want always available, add to write_config() and call it here.
        if (db_orig_version < 4)
        {
            // Prefs need to be reinjected.
            write_config();
        }


        // If it's ok to drop all the data, here's an example to create a new empty table.
        if (db_orig_version < 5)
        {
            g_ss.str(std::string());
            g_ss << "DROP TABLE " << c_str_Tablename_Accounts;
            nb = db.exec(g_ss.str());

            g_ss.str(std::string());
            g_ss << "CREATE TABLE " << c_str_Tablename_Accounts << " ( "
                << "account_id INTEGER PRIMARY KEY,"
                << "broker_id INTEGER,"
                ...
                << "access_secret TEXT"
                << ")";
            nb = db.exec(g_ss.str());
        }
        */

        set_pref(IP_DB_VERSION,c_nDatabaseVersion);

    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR sqlite upgrade_db_as_needed from [v" << db_orig_version << "] to [v" << c_nDatabaseVersion << "]: exec [" << nb << "]: " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }

    g_ss.str(std::string());
    g_ss << "Successfully upgraded sqlite database from [v" << db_orig_version << "] to [v" << c_nDatabaseVersion << "]...";
    log(LV_ALWAYS,g_ss.str());
    endTransaction();
    return true;
}


bool SqliteLocalModel::read_prefs()
{
    startTransaction(false);
    try
    {
        // Read from SQL Preferences table.
        // Use the pref names, to improve table readability.
        // NOTE that this means we need access to the pref init data here.
        // The name comes from this: s_str_pref_config[sp].strName_
        SQLite::Database& db = *pdb_;
        {
            SQLite::Statement query(db, getSelectSQL(c_str_Tablename_PrefInt                ,cv_cols_PrefInt                ));
            while (query.executeStep())
            {
                // OK, obsessing a bit here...
                // We can use a range based for loop, convenient; it totally avoids use of any iterators.
                // BUT if we want a count of where we are, or to reference the prev or next item, we WANT iterators!
                // You can fall back on the pre-C++11 standard for loop, that probably makes the most sense.  Or just use a counter.
                // BUT... there is also this crazy boost helper that combines range based for loop with access to the "index".
                // It's not too bad.
                //
                // ALSO note that enums don't do math.  :-)  Until you provide an operator()++.
                // BUT, many many MANY people suggest using a static cast, and since it's a convoluted pain to "make it better",
                // we'll do the horrible thing and perform a static_cast here.
                // See http://en.cppreference.com/w/cpp/language/enum#Scoped_enumerations
                for (const auto &element : boost::adaptors::index(s_int_pref_config))
                {
                    if (element.value().strName_ == query.getColumn(0).getText())
                    {
                        INT_PREF_INDEX ip = static_cast<INT_PREF_INDEX>(static_cast<int_fast32_t>(IP_FIRST) + element.index());
                        int_prefs_[ip] = query.getColumn(1).getInt();
                        break;
                    }
                }
            }
        }
        {
            SQLite::Statement query(db, getSelectSQL(c_str_Tablename_PrefStr                ,cv_cols_PrefStr                ));
            while (query.executeStep())
            {
                for (const auto &element: boost::adaptors::index(s_str_pref_config))
                {
                    if (element.value().strName_ == query.getColumn(0).getText())
                    {
                        STRING_PREF_INDEX sp = static_cast<STRING_PREF_INDEX>(static_cast<int_fast32_t>(SP_FIRST) + element.index());
                        str_prefs_[sp] = query.getColumn(1).getText();
                        break;
                    }
                }
            }
        }
    }
    catch (std::exception& e)
    {
        // Silently fail; this will cause the database to be created.
        /*
        g_ss.str(std::string());
        g_ss << "ERROR SQLite read_prefs str: " << e.what();
        log(LV_ERROR,g_ss.str());
        */

        endTransaction();
        return false;
    }

    endTransaction();
    return true;
}


bool SqliteLocalModel::write_pref(INT_PREF_INDEX ip)
{
    startTransaction();
    int nb = -299;
    try
    {
        SQLite::Database& db = *pdb_;

        // We just need INSERT OR REPLACE here (http://stackoverflow.com/questions/690632/how-do-i-update-a-row-in-a-table-or-insert-it-if-it-doesnt-exist),
        // not a fancy upsert (http://stackoverflow.com/questions/418898/sqlite-upsert-not-insert-or-replace).
        g_ss.str(std::string());
        g_ss << "INSERT OR REPLACE INTO " << c_str_Tablename_PrefInt << " (" << getColumnList(cv_cols_PrefInt) << ") VALUES( \"" << s_int_pref_config[ip].strName_ << "\", " << get_pref(ip) << " )";
        nb = db.exec(g_ss.str());
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR SQLite write_pref int: exec [" << nb << "]: " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }

    endTransaction();
    return true;
}
bool SqliteLocalModel::write_pref(STRING_PREF_INDEX sp)
{
    startTransaction();
    int nb = -399;
    try
    {
        SQLite::Database& db = *pdb_;

        // We just need INSERT OR REPLACE here (http://stackoverflow.com/questions/690632/how-do-i-update-a-row-in-a-table-or-insert-it-if-it-doesnt-exist),
        // not a fancy upsert (http://stackoverflow.com/questions/418898/sqlite-upsert-not-insert-or-replace).
        g_ss.str(std::string());
        g_ss << "INSERT OR REPLACE INTO " << c_str_Tablename_PrefStr << " (" << getColumnList(cv_cols_PrefStr) << ") VALUES( \"" << s_str_pref_config[sp].strName_ << "\", \"" << get_pref(sp) << "\" )";
        nb = db.exec(g_ss.str());
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR SQLite write_pref str: exec [" << nb << "]: " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }

    endTransaction();
    return true;
}


bool SqliteLocalModel::initializeMaxDbIds()
{
    startTransaction(false);
    try
    {
        SQLite::Database& db = *pdb_;

        // Set all the max db_ids, so we can create new objects purely in memory by incrementing an internal max_db_id.
        { SQLite::Statement query(db, string("SELECT MAX(id) FROM ") + c_str_Tablename_Cars         ); query.executeStep(); Car::Car_max_db_id_ = query.getColumn(0).getInt64();    }
        { SQLite::Statement query(db, string("SELECT MAX(id) FROM ") + c_str_Tablename_Tires        ); query.executeStep(); Tire::Tire_max_db_id_ = query.getColumn(0).getInt64();  }
        { SQLite::Statement query(db, string("SELECT MAX(id) FROM ") + c_str_Tablename_AppUsers     ); query.executeStep(); AppUser::au_max_db_id_ = query.getColumn(0).getInt64(); }

    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR initializeMaxDbIds(): " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    endTransaction();
    return true;
}


bool SqliteLocalModel::readTires()
{
    int_fast32_t count = 0;
    startTransaction(false);
    try
    {
        log(LV_INFO,"Loading tires... ",false,true);
        SQLite::Database& db = *pdb_;
        SQLite::Statement query(db, getSelectSQL(c_str_Tablename_Tires, cv_cols_Tires));
        while (query.executeStep())
        {
            int n = query.getColumn(1).getInt();
            auto itau = findAppUser(n);
            if (itau == users_.end())
            {
                g_ss.str(string());
                g_ss << "ERROR readTires() AppUser [" << n << "] not found";
                log(LV_ERROR,g_ss.str());
                return false;
            }
            AppUser& au = **itau;

            n = query.getColumn(2).getInt();
            auto itc = au.findCar(n);
            if (itc == au.cars_.end())
            {
                g_ss.str(string());
                g_ss << "ERROR readTires() Car [" << n << "] not found";
                log(LV_ERROR,g_ss.str());
                return false;
            }
            Car& car = **itc;

            // Directly insert each into memory.
            car.addTireToMemory(
                new Tire
                (
                    query.getColumn(0).getInt(),                            // {"id"        ,"INTEGER PRIMARY KEY NOT NULL,"},
                                                                            // {"appuser_id","INTEGER NOT NULL,"            },
                    car,                                                    // {"car_id"    ,"INTEGER NOT NULL,"            },
                    query.getColumn(3).getText(),                           // {"name"      ,"TEXT,"                        },
                    query.getColumn(4).getText()                            // {"type"      ,"TEXT,"                        }
                )
            );
            ++count;
        }
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << endl << "ERROR SQLite readTire(): " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    log(LV_INFO,boost::lexical_cast<string>(count) + " loaded.");
    endTransaction();
    return true;
}
bool SqliteLocalModel::writeTire(Tire& t)
{
    startTransaction();
    try
    {
        SQLite::Database& db = *pdb_;
        g_ss.str(std::string());
        g_ss << getInsertSQLStart(c_str_Tablename_Tires,cv_cols_Tires)
            << t.db_id_         << ", "    // {"id"        ,"INTEGER PRIMARY KEY NOT NULL,"},
            << t.car_.db_id_    << ", "    // {"car_id"    ,"INTEGER NOT NULL,"            },
            << "\"" << t.name_  << "\", "  // {"name"      ,"TEXT,"                        },
            << "\"" << t.type_  << "\" "   // {"type"      ,"TEXT,"                        }
            << " )";
       db.exec(g_ss.str());
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR SQLite writeTire(): " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    endTransaction();
    return true;
}
bool SqliteLocalModel::deleteTire(Tire& t)
{
    startTransaction();
    try
    {
        SQLite::Database& db = *pdb_;
        g_ss.str(std::string());
        g_ss << "DELETE FROM " << c_str_Tablename_Tires << " WHERE "
            << "id = " << t.db_id_;
        db.exec(g_ss.str());
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR SQLite deleteTire(): " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    endTransaction();
    return true;
}


bool SqliteLocalModel::readCars()
{
    int_fast32_t count = 0;
    int n;
    string str;
    startTransaction(false);
    try
    {
        log(LV_INFO,"Loading cars... ",false,true);
        SQLite::Database& db = *pdb_;
        SQLite::Statement query(db, getSelectSQL(c_str_Tablename_Cars             ,cv_cols_Cars             ));
        while (query.executeStep())
        {
            n = query.getColumn(1).getInt();
            auto itau = findAppUser(n);
            if (itau == users_.end())
            {
                g_ss.str(string());
                g_ss << "ERROR readCars() AppUser [" << n << "] not found";
                log(LV_ERROR,g_ss.str());
                return false;
            }
            AppUser& au = **itau;

            au.addCarToMemory(
                new Car
                (
                    query.getColumn(0).getInt(),    // {"id"           ,"INTEGER PRIMARY KEY NOT NULL,"},
                    au,                             // {"appuser_id"   ,"INTEGER NOT NULL,"            },
                    query.getColumn(2).getText()    // {"name"         ,"TEXT"                         }
                )
            );
            ++count;
        }
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << endl << "ERROR SQLite readCars(): " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    catch (...)
    {
        cout << endl << "WTF" << endl;
        endTransaction();
        return false;
    }
    log(LV_INFO,boost::lexical_cast<string>(count) + " loaded.");
    endTransaction();
    return true;
}
bool SqliteLocalModel::writeCar(Car& c)
{
    startTransaction();
    try
    {
        SQLite::Database& db = *pdb_;
        g_ss.str(std::string());
        g_ss << getInsertSQLStart(c_str_Tablename_Cars,cv_cols_Cars,true)
            << c.db_id_           << ", "     // {"id"           ,"INTEGER PRIMARY KEY NOT NULL,"},
            << c.au_.db_id_       << ", "     // {"appuser_id"   ,"INTEGER NOT NULL,"            },
            << "\"" << c.name_    << "\" "    // {"name"         ,"TEXT"                         }
            << " )";
       db.exec(g_ss.str());
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR SQLite writeCar(): " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    endTransaction();
    return true;
}
bool SqliteLocalModel::deleteCar(Car& c)
{
    startTransaction();
    try
    {
        SQLite::Database& db = *pdb_;
        g_ss.str(std::string());
        g_ss << "DELETE FROM " << c_str_Tablename_Cars << " WHERE "
            << "id = " << c.db_id_;
        db.exec(g_ss.str());
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR SQLite deleteCar(): " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    endTransaction();
    return true;
}


bool SqliteLocalModel::readAppUsers()
{
    int_fast32_t count = 0;
    startTransaction(false);
    try
    {
        log(LV_INFO,"Loading users... ",false,true);
        SQLite::Database& db = *pdb_;
        {
            SQLite::Statement query(db, getSelectSQL(c_str_Tablename_AppUsers               ,cv_cols_AppUsers               ));
            while (query.executeStep())
            {
                // Directly insert each user we find into memory.
                addAppUserToMemory
                (
                    new AppUser
                    (
                        query.getColumn(0).getInt(),                            // << "id INTEGER PRIMARY KEY NOT NULL,"                       // int_fast32_t id,
                        this,
                        query.getColumn(1).getText(),                           // << "username TEXT,"                                         // string str_username_;
                        query.getColumn(2).getText()                            // << "password TEXT,"                                         // string str_password_;
                    )
                );
                ++count;
            }
        }
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << endl << "ERROR SQLite readAppUsers(): " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    log(LV_INFO,boost::lexical_cast<string>(count) + " loaded.");
    endTransaction();
    return true;
}
bool SqliteLocalModel::writeAppUser(AppUser& au)
{
    startTransaction();
    try
    {
        SQLite::Database& db = *pdb_;
        g_ss.str(std::string());
        g_ss << getInsertSQLStart(c_str_Tablename_AppUsers,cv_cols_AppUsers,true)
            << au.db_id_                        << ", "        //     {"id"                     ,"INTEGER PRIMARY KEY NOT NULL,"  },
            << "\"" << au.username()            << "\", "      // << "username TEXT,"                                         // string str_username_;
            << "\"" << au.password()            << "\" "       // << "password TEXT,"                                         // string str_password_;
            << " )";
       db.exec(g_ss.str());
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR SQLite writeAppUser(): " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    endTransaction();
    return true;
}
bool SqliteLocalModel::deleteAppUser(AppUser& au)
{
    startTransaction();
    try
    {
        SQLite::Database& db = *pdb_;
        g_ss.str(std::string());
        g_ss << "DELETE FROM " << c_str_Tablename_AppUsers << " WHERE "
            << "id = " << au.db_id_;
        db.exec(g_ss.str());
    }
    catch (std::exception& e)
    {
        g_ss.str(std::string());
        g_ss << "ERROR SQLite deleteAppUser(): " << e.what();
        log(LV_ERROR,g_ss.str());
        endTransaction();
        return false;
    }
    endTransaction();
    return true;
}


// We need to perform maintenance once a day.
// It is our responsibility to only perform maintenance once if this is called more than once.
// We return true if we actually performed maintenance.
bool SqliteLocalModel::performAfterHoursMaintenance(const ptime& pt)
{
    using namespace boost::filesystem;

    // TODO
    /*

        log(LV_ALWAYS,"*** AFTER HOURS MAINTENANCE DONE ***");
        return true;
    */

    return false;
}


bool SqliteLocalModel::backup(const string& prefix, const string& suffix)
{
    return backup_any_old_file(s_str_db_name,prefix,suffix);
}


string SqliteLocalModel::getCreateSQL(const string& table, const vector<pair<string,string>>& cols)
{
    stringstream ss2;
    ss2 << "CREATE TABLE " << table << " ( ";
    for (auto itcol = cols.begin(); itcol != cols.end(); ++itcol)
    {
        ss2 << itcol->first << " " << itcol->second;
    }
    ss2 << ")";
    return ss2.str();
}
string SqliteLocalModel::getColumnList(const vector<pair<string,string>>& cols)
{
    stringstream ss;
    for (auto itcol = cols.begin(); itcol != cols.end(); ++itcol)
    {
        if (itcol != cols.begin()) { ss << "," ; }
        ss << itcol->first;
    }
    return ss.str();
}
string SqliteLocalModel::getSelectSQL(const string& table, const vector<pair<string,string>>& cols)
{
    stringstream ss;
    ss << "SELECT " << getColumnList(cols) << " FROM " << table;
    return ss.str();
}
string SqliteLocalModel::getInsertSQLStart(const string& table, const vector<pair<string,string>>& cols, bool bOrReplace)
{
    stringstream ss;
    ss << "INSERT" << (bOrReplace?" OR REPLACE":"") << " INTO " << table << " (" << getColumnList(cols) << ") VALUES( ";
    return ss.str();
}
int SqliteLocalModel::migrateTable(const string& table, const vector<pair<string,string>>& cols)
{
    // Make a new temp table with the new format.
    string temptable = table+"migrated";
    SQLite::Database& db = *pdb_;
    int nb = db.exec(getCreateSQL(temptable,cols));

    // Copy everything from original table to the temp one.
    stringstream ss;
    ss << "INSERT INTO " << temptable << " (" << getColumnList(cols) << ") SELECT " << getColumnList(cols) << " FROM " << table;
    nb = db.exec(ss.str());

    // Drop the original table.
    g_ss.str(std::string());
    g_ss << "DROP TABLE " << table;
    nb = db.exec(g_ss.str());

    // Rename the temp to the official name.
    g_ss.str(std::string());
    g_ss << "ALTER TABLE " << temptable << " RENAME TO " << table;
    nb = db.exec(g_ss.str());

    return nb;
}


#include "SqliteLocalModelHttpThread.cpp"
