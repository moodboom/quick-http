/*
 Copyright (c) A better Software http://abettersoftware.com
 All rights reserved.
*/

#ifndef VERSION_H
#define VERSION_H

/*

	VERSIONING

    "Release m.c"                   official name

    mm.cc.rr.ddd                    full version string

    Setup_m.c.r.exe (etc)           build names; No need for d as r will always be incremented on publish

    mm  = major version				haven't hit v1.0 yet

    cc  = compatibility version		only change this when breaking compatibility with previous version;
									any client-server combination with x.x.-.-.- the same will work together

    rr  = release version			update whenever a new stable release is made

    ddd = database version			Bump database version for any database structure change!
                                    This doesn't necessarily mean incompatibility, we can upgrade.
                                    Coordinate these functions!
                                        create_database()
                                        update_schema_as_needed()
                                        read|write_#object#()

    bbb = build version				change this whenever a new build with ANY code change occurs
									actually we're not going to track each build, too much
									just make sure release# is always incremented on every publish

*/

// =======================================
// VERSION HISTORY
// =======================================
//  0.1.000.001    2015/01/16 initially created
//  0.1.000.002    2015/03/06 getting https client working
//  0.1.000.003    2015/04/27 first implementation of actual model schema
//  0.1.000.004    2015/04/27 trader model schema bump; adding hardcoded values
//  0.1.000.005    2015/05/24 sqlite db coming to life; adding prefs, accounts
//  0.1.000.006    2015/05/24 adding persistent access_key+secret to account
//  0.1.000.007    2015/05/24 adding primary key to account
//  0.1.000.008    2015/05/25 adding pin to account
//  0.1.000.009    2015/05/26 three new tables, might need to recreate db to get NOT NULL on primary keys!  oops!
//  0.2.000.010    2015/05/26 trash the old versions, they ALL used primary keys that allowed NULL, nip that now!  set up c_nLastCompatibleDBVersion.
//  0.3.000.011    2015/05/26 AutotradedStocks missing quantity, doh.  Rebreak it since we just broke.
//  0.3.000.012    2015/06/02 Adding rebuy fields to Account, StockPick
//  0.4.000.013    2015/06/02 Adding b_active; converting all "x1000" integers to REAL/getDouble(), enough to break from previous (we have not released yet)
//  0.5.000.014    2015/06/05 Rework of all trading objects to include preloaded settings for subsequently created objects;
//                              eg AutotradedStock includes what it needs to create rebuy StockPick
//  0.6.000.015    2015/06/05 Typo in creating Accounts table (missing comma, ugg)
//  0.6.001.015    2015/06/05 First release with a good set of autotrade variables; let's call it release 0.6.1
//  0.6.001.016    2015/06/09 Need to track both sandbox and prod keys, use according to startup params (whether "test" is provided or not)
//  0.6.001.017    2015/06/09 Needed missing migration code
//  0.6.001.018    2015/06/09 Still getting prefs migration going
//  0.7.000.019    2015/06/11 StockPick rework
//  0.8.000.020    2015/06/12 Changed rank/account to account/rank in SP.  Getting used to breaking compatibility for now, faster than writing upgrade db code.
//  0.9.000.021    2015/06/19 Added AppUsers table, pulled parts from Accounts table.
//  0.9.000.022    2015/06/21 Changed from quotes account to quotes appuser.  Fixed test data to use "test account".
//  0.9.000.023    2015/06/21 Syncing object members with etrade API fields, for account/order/portfolio validations.
//  0.9.000.024    2015/06/25 Major refactor, no more stock order object.
// 0.10.000.024    2015/06/26 Moving autotrade params into one class that can be passed around from SP to AS to SP...
// 0.10.000.025    2015/06/26 Creating AutotradeParameterSet was a major refactor.  Again.
// 0.10.000.026    2015/06/28 Account fields fix.
// 0.10.000.027    2015/06/30 Added StockQuotes table
// 0.10.000.028    2015/07/02 AutotradeParameterSet refactor to map; need to ensure a default set is written in each new db.
// 0.10.000.029    2015/07/02 Stock quote history now includes optional account and buy/sell status.
// 0.10.000.030    2015/07/02 Major part of [set with db_id hash] refactor is complete.  Most tables have id PRIMARY KEY now.  More may come later.
// 0.10.000.031    2015/07/06 Added active status to SP and AS.  They now stay in memory and db as they flip back and forth.  The pair is only removed when AS sells as a non-rebuy.
// 0.10.000.032    2015/07/06 Added ba.d_autotraded_value_change_
// 0.10.000.033    2015/07/07 Added AccountHistory, AccountHistoryStock
// 0.10.001.034    2015/07/07 Lots of fixes, we have a D3 graph now, and we'll publish it to abettertrader.com
// 0.10.001.035    2015/07/07 Starting dynamic stopsell work
// 0.10.001.036    2015/07/11 More dynamic stopsell work database changes; refactor to use table fields in a const string vector for CREATE/SELECT/INSERT
// 0.11.000.037    2015/07/11 inconsistent column name, no "b_" on rebuy_on_stopsell - no column rename, so it's another breaking change; also, new graph logic so let's set old code to incompatible
// 0.11.000.038    2015/07/11 Added AS.b_stage2_
// 0.11.000.039    2015/07/11 WHOOPS update code mistakenly added b_stage2 to APS, ha.  Adding to AS this time.
// 0.11.000.040    2015/07/13 SP db-only b_initialized value - allows us to enter a new SP in db and have it automatically initialized, but avoid if SP has already been init'ed.
// 0.12.000.041    2015/07/13 Breaking change to allow fast SP manual add to db: move forward and set NOT NULL all mandatory SP fields
// 0.13.000.042    2015/07/14 Breaking change to: add AS+SP history; moving towards delayed-write of object changes at end of get_quotes(), not there yet.
// 0.14.000.043    2015/07/15 Breaking change adding db_id and using as primary key for Account - all db_id-keyed objects now have a base class
// 0.14.000.043    2015/07/17 ADDED THIS: migrate_table() -- it allows deeper table changes to be handled by upgrade_db(), sweet!  Going back and migrating v42 with it.
// 0.14.000.044    2015/07/17 Added db description pref.  Added to write_config() so user can update it.  It's posted on startup.
// 0.14.001.044    2015/07/22 Tightened default brackets.
// 0.14.001.045    2015/07/22 Added SPT_RISE; added rebuy_type in APS; SPT_RISE used as default value.
// 0.14.001.046    2015/07/23 Added AS b_initialized (and lots of "NOT NULL"s on required fields), BA d_minimum_purchase_size; renamed stupid-assed BA d_autotraded_value_change to d_available_cash
// 0.14.002.046    2015/07/29 Dropped d_bump_percent from 1.01 to 1.007.  Might need to add two types, one for new and one for rebuy.
// 0.14.002.047    2015/08/02 Added SPASStartEnd for analysis data.
// 0.14.002.048    2015/08/03 Removed timestamp from SPASStartEnd, it was redundant.  Also corrected extraction of analysis data to it from source dbs.
// 0.14.002.049    2015/08/04 Added SP "stage 0" b_track_only.
// 0.14.002.050    2015/08/08 Renamed SPASHistory to SPASBracketEvents, and started tracking them in memory for increased performance during analysis.
// 0.14.002.051    2015/08/10 Added APS analysis_percent_change to store results of "last" analysis.
// 0.14.002.052    2015/08/13 Added deleteDupeQuotes() and running it on all older dbs.  They were full of them.
// 0.14.002.053    2015/08/16 Added APS symbol and timestamp fields to help us track results from analysis
// 0.14.002.054    2015/08/20 Added APS b_analyze_and_update_; we will auto-update any prod APS with this flag set after analysis runs
// =======================================
const int c_nMajorVersion =         0;
const int c_nCompatibilityVersion =     14;                 // bump on CODE (not just db) incompatibility (ie, significant new logic); rolls back to zero on major bump
const int c_nReleaseVersion =               2;              // rolls back to zero on compatibility bump
const int c_nDatabaseVersion =                  54;         // never rolls back to zero; coordinate: create_database() | update_schema_as_needed() | read|write_#object#()
const int c_nLastCompatibleDBVersion =          42;         // we don't load db's older than this; update this number when c_nCompatibilityVersion changes


// Our version string looks like this: 1.2.003.004
static std::string get_version_str()
{
    stringstream ss;
    ss << std::setfill( '0' ) << c_nMajorVersion << "." << c_nCompatibilityVersion << "." << std::setw( 3 ) << c_nReleaseVersion << "." << std::setw( 3 ) << c_nDatabaseVersion;
    return ss.str();
}

#endif // VERSION_H
