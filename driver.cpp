#include "main.h"
#include "driver.h"
#include "util.h"

driver_t::driver_t(){
	count = 0;
}

driver_t::~driver_t(){
	if(count != 0){
		print("driver_t closing with still-valid data", P_ERR);
	}
}

uint64_t driver_t::get_count(){
	lock.lock();
	const uint64_t count_ = count;
	lock.unlock();
	return count_;
}

void driver_t::set_count(uint64_t count_){
	lock.lock();
	count = count_;
	lock.unlock();
}
