#include "main.h"
#include "util.h"
#include "deposit.h"
#include "tx.h"
#include "json_rpc.h"
#include "settings.h"
#include "lock.h"

static std::vector<fee_t> fee;
static lock_t fee_lock;

void deposit::add_fee(fee_t fee_){
	if(fee_.get_percent() > .25){
		print("fee is absurdly high, force with --force-fee", P_CRIT);
		// should be done at runtime, so it is safe to throw an exception
	}
	LOCK_RUN(fee_lock, [](fee_t fee_){
			for(unsigned int i = 0;i < fee.size();i++){
				bool addr = fee[i].get_address() == fee_.get_address();
				bool perc = fee[i].get_percent() == fee_.get_percent();
				bool update = addr && !perc;
				if(update){
					print("updating fee, old one should have been deleted", P_ERR);
					fee[i].set_percent(fee_.get_percent());
				}
			}
		}(fee_));
}


int deposit::send(std::string address, satoshi_t satoshi, int type){
	satoshi_t working_satoshi = satoshi;
	LOCK_RUN(fee_lock, [](satoshi_t *working_satoshi){
			for(unsigned int i = 0;i < fee.size();i++){
				const long double fee_percent = fee[i].get_percent();
				const satoshi_t fee_size = fee_percent*(*working_satoshi);
				if(fee_size == 0){
					continue;
				}
				if(fee[i].get_address() == ""){
					continue;
				}
				tx_out_t tx_tmp(fee[i].get_address(), fee_size);
				print("adding the fee of " + std::to_string(fee_size) + " satoshi (" + std::to_string(fee_percent) + ") to " + fee[i].get_address(), P_NOTICE);
				tx::add_tx_out(tx_tmp);
				*working_satoshi -= fee_size;
			}
		}(&working_satoshi));
	tx::add_tx_out(tx_out_t(address, working_satoshi));
	return 0;
}

int deposit::init(){
	/*
	  TODO: zero-one-infinity this
	 */
	const bool fee_1 = settings::get_setting("fee_1_address") != "" && settings::get_setting("fee_1_percent") != "";
	const bool fee_2 = settings::get_setting("fee_2_address") != "" && settings::get_setting("fee_2_percent") != "";
	const bool fee_3 = settings::get_setting("fee_3_address") != "" && settings::get_setting("fee_3_percent") != "";
	const bool fee_4 = settings::get_setting("fee_4_address") != "" && settings::get_setting("fee_4_percent") != "";
	try{
		if(fee_1){
			add_fee(fee_t(settings::get_setting("fee_1_address"), std::stold(settings::get_setting("fee_1_percent"))));
		}
		if(fee_2){
			add_fee(fee_t(settings::get_setting("fee_2_address"), std::stold(settings::get_setting("fee_2_percent"))));
		}
		if(fee_3){
			add_fee(fee_t(settings::get_setting("fee_3_address"), std::stold(settings::get_setting("fee_3_percent"))));
		}
		if(fee_4){
			add_fee(fee_t(settings::get_setting("fee_4_address"), std::stold(settings::get_setting("fee_4_percent"))));
		}
	}catch(std::invalid_argument e){
		print("invalid argument for add_fee, check for typos in settings.cfg", P_ERR);
	}catch(std::out_of_range e){
		print("out of range for add_fee, check for typos in settings.cfg", P_ERR);
	}catch(std::exception e){
		print("unknown exception for add_fee: " + (std::string)e.what(), P_ERR);
	}
	return 0;
}

int deposit::close(){
	return 0;
}

std::string fee_t::get_address(){
	return address;
}

void fee_t::set_address(std::string address_){
	address = address_;
}

long double fee_t::get_percent(){
	return percent;
}

void fee_t::set_percent(long double percent_){
	percent = percent_;
}

fee_t::fee_t(std::string address_, long double percent_){
	set_address(address_);
	set_percent(percent_);
}

fee_t::~fee_t(){}
