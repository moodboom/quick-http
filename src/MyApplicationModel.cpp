/*
 * MyApplicationModel.cpp
 *
 *  Created on: Oct 22, 2015
 *      Author: m
 */

#include <utilities.hpp>            // For log()
#include "MemoryModel.hpp"          // For g_p_local, used to write our objects to db
#include "MyApplicationModel.hpp"


// order: Tire, Car


// ------------------------------------------------------------------------------
// CONSTANTS GLOBALS STATICS
// ------------------------------------------------------------------------------

// ====
// KEYS
// ====
// These static globals are used as lookup keys for unsorted_set::find(key).
AppUser g_AUKey(0,0,"key");
Car g_CarKey(0,g_AUKey,"key");
Tire g_TireKey(0,g_CarKey,"key");
// ====

// ==========
// max_db_ids
// ==========
// These are tracked in-memory so we can create new objects without hitting db.
int64_t AppUser::au_max_db_id_ = -1;
int64_t Tire::Tire_max_db_id_ = -1;
int64_t Car::Car_max_db_id_ = -1;
// ==========

// ------------------------------------------------------------------------------


Tire::Tire(
    int64_t db_id,
	Car& car,
    string name,
    string type
) :
    // Call base class
    inherited(db_id),

    // init vars
	car_    (car    ),
    name_   (name   ),
    type_   (type   )
{
    assignNewDbIdAsNeeded();
}


Car::Car(
    int64_t db_id,
    AppUser& au,
    string name,
    bool b_active
) :
    // Call base class
    inherited(db_id),

    // Init vars
    au_         (au         ),
    name_	    (name       ),
    b_active_	(b_active	)
{
}


Car::~Car()
{
    clearTires();
}


// This is called for a complete cleanup
void Car::clearTires()
{
    for (auto pTire : tires_)
        delete pTire;
    tires_.clear();
}


void Car::setActive(bool active)
{
    b_active_ = active;
    setDirty();
}
void Car::deactivate()
{
    ::log(LV_INFO,"Deactivating this car: ",false,true);
    log(LV_INFO,false);

    setActive(false);
}


Tire& Car::addTire(Tire* pTire)
{
    if (!au_.pmm_->writeTire(*pTire))
        log(LV_ERROR,"ERROR writing Tire!");

    return addTireToMemory(pTire);
}
Tire& Car::addTireToMemory(Tire* pTire)
{
    // We need a valid db id.
    assert(pTire->db_id_ != -1);

    tires_.insert(pTire);
    return *pTire;
}
TireIt Car::findTire(int64_t id)
{
    g_TireKey.db_id_ = id;
    return tires_.find(&g_TireKey);
}


void Car::log(LOG_TO_FILE_VERBOSITY verb, bool bSuppressNewline)
{
    g_ss.str(std::string());
    g_ss << boost::format("[%s]") % name_;
    ::log(verb,g_ss.str(),false,bSuppressNewline);
}


Car& AppUser::addCar(Car* pCar)
{
    if (!pmm_->writeCar(*pCar))
        ::log(LV_ERROR,"ERROR writing Car!");

    return addCarToMemory(pCar);
}
Car& AppUser::addCarToMemory(Car* pCar)
{
    // We need a valid db id.
    assert(pCar->db_id_ != -1);

    cars_.insert(pCar);
    return *pCar;
}
CarIt AppUser::findCar(int64_t id)
{
    g_CarKey.db_id_ = id;
    return cars_.find(&g_CarKey);
}


