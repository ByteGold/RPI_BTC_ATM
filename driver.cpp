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
