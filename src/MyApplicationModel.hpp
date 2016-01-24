/*
 * MyApplicationModel.hpp
 *
 *  Created on: Oct 22, 2015
 *      Author: m
 */

#pragma once

#include <utilities.hpp>
#include <STLContainers.h>    // For PersistentIDObject

class Car;             // defined below, last - but first used for parent refs in child objects


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------

// ------------------------------------------------------------------------------


class Tire : public PersistentIDObject
{
typedef PersistentIDObject inherited;

public:
    Tire(
        int_fast32_t db_id,
		Car& car,
        string name = "default",
		string type = "Pirelli"
    );

    Car& car_;        // Our owner.

    string name_;
    string type_;

    virtual void assignNewDbIdAsNeeded() { if (db_id_ == -1) { ++Tire_max_db_id_; db_id_ = Tire_max_db_id_; setDirty(); } }
    static int_fast64_t Tire_max_db_id_;
};

// ========================================================
// Tire container
// ========================================================
typedef std::unordered_set<Tire*,PersistentIDObject_hash,PersistentIDObjects_equal > Tires;
typedef Tires::iterator TireIt;
// ========================================================


class AppUser;
class Car : public PersistentIDObject
{
typedef PersistentIDObject inherited;

public:

    Car(
        int_fast32_t db_id,
        AppUser& au,
        string name,
        bool b_active = true
    );

    virtual ~Car();

    bool bActive() const { return b_active_; }
    void setActive(bool active = true);
    void deactivate();

    bool analyze();

    void clearTires();

    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
    // Manage contained objects.
    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
    Tire& addTire(Tire* pd);
    Tire& addTireToMemory(Tire* pd);
    TireIt findTire(int_fast32_t db_id);
    bool removeTire(TireIt itd);
    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

    string name_;

    void log(LOG_TO_FILE_VERBOSITY verb, bool bSuppressNewline = true);

    // Our owner.
    AppUser& au_;

    Tires tires_;

    virtual void assignNewDbIdAsNeeded() { if (db_id_ == -1) { ++Car_max_db_id_; db_id_ = Car_max_db_id_; setDirty(); } }
    static int_fast64_t Car_max_db_id_;

protected:

    mutable bool b_active_;
};
// ========================================================
// Car container
// ========================================================
typedef std::unordered_set<Car*,PersistentIDObject_hash,PersistentIDObjects_equal > Cars;
typedef Cars::iterator CarIt;
// ========================================================


// AppUser's parent, defined later.
class MemoryModel;

class AppUser : public PersistentIDObject
{
typedef PersistentIDObject inherited;

public:
    AppUser(
        int_fast32_t db_id,
        MemoryModel* pmm,
        string str_username = "",
        string str_password = "",
        bool b_active = true
    ) :
        // Call base class
        inherited(db_id),

        // Init vars.
        pmm_(pmm),
        str_username_(str_username),
        str_password_(str_password),
        b_active_    (b_active)
    {}

    bool bActive() const { return b_active_; }
    void setActive(bool active = true);
    void deactivate();

    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
    // Manage contained objects.
    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
    Car& addCar(Car* pcar);
    Car& addCarToMemory(Car* pcar);
    CarIt findCar(int_fast32_t db_id);
    bool removeCar(CarIt it);
    // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

    // Getters, sigh
    const string& username()            const { return str_username_            ; }
    const string& password()            const { return str_password_            ; }

    void log(LOG_TO_FILE_VERBOSITY verb, bool bSuppressNewline = true);

    MemoryModel* pmm_;
    Cars cars_;

    virtual void assignNewDbIdAsNeeded() { if (db_id_ == -1) { ++au_max_db_id_; db_id_ = au_max_db_id_; setDirty(); } }
    static int_fast64_t au_max_db_id_;

protected:

    void clearCars();

    string str_username_;
    string str_password_;

    mutable bool b_active_;
};
// ========================================================
// AppUser container
// ========================================================
typedef std::unordered_set<AppUser*,PersistentIDObject_hash,PersistentIDObjects_equal > AppUsers;
typedef AppUsers::iterator AppUserIt;
// ========================================================


// ========================================================
// KEYS
// ========================================================
// These static globals are used as lookup keys for unsorted_set::find(key).
// ========================================================
extern Tire g_TireKey;
extern Car g_CarKey;
extern AppUser g_AUKey;
// ========================================================


