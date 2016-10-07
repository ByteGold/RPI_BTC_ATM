/*
  Nothing needs to be initalized, so don't bother with that
 */
#include "main.h"
#include "util.h"
#include "settings.h"
#include "atm.h"
#include "qr.h"
#include "tx.h"
#include "driver.h"
#include "ch_926.h"
#include "deposit.h"

static std::vector<driver_t*> drivers;

/*
  Procedure:
  1. Address is read from QR API/NFC API (NFC is pretty based IMHO)
  2. Once all of the runnable code has ran, prompt for input (fetching 
  rates live, getting settings, no tomfoolery
  3. Coins are accepted until a pre-defined timeout or event (button)
  4. Deposit is made with the deposit API (deposit only pays fees that are 
  defined by the operator, it doesn't do anything else, the TX api takes care
  of that)
  4. Transaction is divided up properly and send to the TX API
*/

static std::string atm_qr_from_webcam(){
	std::string address = qr::read_from_webcam();
	if(address != "" && address.substr(0, 8) == "bitcoin:"){
		address = address.substr(8, address.size());
		print("removed 'bitcoin:' prefix from address (" + address + ")", P_NOTICE);
	}
	return address;
}

static uint64_t atm_total_driver_count(uint64_t deposit_timeout){
	uint64_t old_count = 0;
	uint64_t last_change = std::time(nullptr);
	bool accepting = true;
	while(accepting){
		for(unsigned int i = 0;i < drivers.size();i++){
			LOCK_RUN(drivers[i]->lock,
				 const uint64_t d_cnt = drivers[i]->get_count();
				 if(d_cnt != old_count){
					 old_count += d_cnt;
					 drivers[i]->set_count(0);
					 last_change = std::time(nullptr);
				 });
		}
		accepting = std::time(nullptr)-last_change >= deposit_timeout;
	}
	return old_count;
}

// gets the currency from settings.cfg
// gets the local bitcoin rate via get_btc_rate
// gets the deposit timeout from settings.cfg

static void atm_pre_deposit_code(std::string *currency,
				 long double *local_btc_rate,
				 uint64_t *deposit_timeout){
	try{
		*currency = settings::get_setting("currency");
	}catch(std::exception e){
		pre_pro::exception(e, "get_setting", P_ERR);
		throw e;
	}
	P_V_S(*currency, P_DEBUG);
	try{
		*local_btc_rate = get_btc_rate(*currency);
	}catch(std::exception e){
		pre_pro::exception(e, "get_btc_rate", P_ERR);
		throw e;
	}
	try{
		const uint64_t low_in_std =
			std::stoul(settings::get_setting("atm_low_in_std"));
		*local_btc_rate *= low_in_std;
	}catch(std::runtime_error e){
		if(*currency == "USD" || *currency == "usd"){
			print("low_in_std not defined and using USD, fixing",
			      P_WARN);
			*local_btc_rate *= 100; // penny
		}else{
			throw e;
		}
	}catch(std::exception e){
		pre_pro::exception(e, "low_in_std", P_ERR);
		throw e;
	}
	try{
		*deposit_timeout =
			std::stoul(settings::get_setting("atm_deposit_timeout"));
	}catch(std::runtime_error e){
		print("deposit_timeout not defined, setting to 10", P_WARN);
		*deposit_timeout = 10;
	}catch(std::exception e){
		pre_pro::exception(e, "deposit_timeout", P_ERR);
		throw e;
	}
}

// counts all of the money from all of the drivers and resets
// all of them to zero
// No plan of action for FUBAR, but exceptions should solve most
// problems for us

static void atm_deposit_code(uint64_t *native_vol,
			     uint64_t *deposit_timeout){
	*native_vol = atm_total_driver_count(*deposit_timeout);
}

// generates a satoshi volume and feeds that into the
// deposit API, where the fees as defined by the operator
// are implemented and sent as tx_out_t structures to the
// tx API

static void atm_post_deposit_code(satoshi_t *satoshi_vol,
				  uint64_t *native_vol,
				  long double *local_btc_rate,
				  std::string address){
	*satoshi_vol = *native_vol*(*local_btc_rate);
	P_V(*satoshi_vol, P_DEBUG);
	try{
		deposit::send(address, *satoshi_vol);
	}catch(std::exception e){
		pre_pro::exception(e, "deposit::send", P_ERR);
		throw e;
	}
}

/*
  returns transacted volume in satoshis on success
  negative on errors (but should be an exception)
 */

satoshi_t atm::loop(){
	const std::string address = atm_qr_from_webcam();
	if(address == ""){
		return 0;
	}
	std::string currency;
	long double local_btc_rate = 0; // in lowest local denomination
	uint64_t deposit_timeout = 0;
	uint64_t native_vol = 0;
	satoshi_t satoshi_vol = 0;
	try{
		atm_pre_deposit_code(&currency,
				     &local_btc_rate,
				     &deposit_timeout);
	}catch(std::exception e){
		pre_pro::exception(e, "atm_pre_deposit_code", P_ERR);
		throw e;
	}
	try{
		atm_deposit_code(&native_vol, &deposit_timeout);
	}catch(std::exception e){
		pre_pro::exception(e, "atm_deposit_code", P_ERR);
		throw e;
	}
	try{
		atm_post_deposit_code(&satoshi_vol,
				      &native_vol,
				      &local_btc_rate,
				      address);
	}catch(std::exception e){
		pre_pro::exception(e, "atm_post_deposit_code", P_ERR);
		throw e;
	}
	return satoshi_vol;
}

void atm::init(){
	// TODO: convert from parameters to cfg file
	if(settings::get_setting("ch_926_drv") == "true"){
		print("Enabling CH 926 driver", P_NOTICE);
		driver_t *ch_926 = new driver_t;
		ch_926->init = ch_926_init;
		ch_926->close = ch_926_close;
		ch_926->run = ch_926_run;
		ch_926->reject_all = ch_926_reject_all;
		ch_926->accept_all = ch_926_accept_all;
		drivers.push_back(ch_926);
	}
	for(unsigned int i = 0;i < drivers.size();i++){
		drivers[i]->init();
	}
	// doesn't need the lock, haven't built a function that handles
	// functions with commas or other special characters
	threads.emplace_back(std::thread([](){
		while(running){
			for(unsigned int i = 0;i < drivers.size();i++){
				LOCK_RUN(drivers[i]->lock,
					 drivers[i]->run(
						 &(drivers[i]->count)));
			}
			sleep_ms(DEFAULT_THREAD_SLEEP);
		}
	}));
}

void atm::close(){
	for(unsigned int i = 0;i < drivers.size();i++){
		drivers[i]->close();
		delete drivers[i];
	}
	drivers.clear();
}
