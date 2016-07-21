#include "main.h"
#include "util.h"
#include "deposit.h"
#include "tx.h"
#include "json_rpc.h"
#include "settings.h"

static std::vector<fee_t> fee;
static std::mutex fee_lock;

void deposit::add_fee(fee_t fee_){
  if(fee_.get_percent() > .25){
    print("fee is absurdly high, force with --force-fee", P_CRIT);
    // should be done at runtime, so it is safe to throw an exception
  }
  LOCK_RUN(fee_lock, [](fee_t fee_){
      for(unsigned int i = 0;i < fee.size();i++){
	bool addr = fee[i].get_address() == fee_.get_address();
	bool perc = fee[i].get_percent() == fee_.get_percent();
	if(addr){
	  if(perc){
	    break;
	  }else{
	    print("updating fee, old one should have been deleted", P_ERR);
	    fee[i].set_percent(fee_.get_percent());
	  }
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
	if(fee_size != 0){
	  if(fee[i].get_address() != ""){
	    tx_out_t tx_tmp(fee[i].get_address(), fee_size);
	    print("adding the fee of " + std::to_string(fee_size) + " satoshi (" + std::to_string(fee_percent) + ") to " + fee[i].get_address(), P_NOTICE);
	    tx::add_tx_out(tx_tmp);
	  }else{
	    print("applying standard fee, not making a transaction", P_NOTICE);
	  }
	}
	*working_satoshi -= fee_size;
      }
    }(&working_satoshi));
  tx::add_tx_out(tx_out_t(address, working_satoshi));
  if(settings::get_setting("no_tx_blocks") == "true"){
    tx::send_transaction_block();
  }
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
