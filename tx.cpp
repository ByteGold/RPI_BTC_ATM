/*
  TODO: Scracth almost all of this code and replace it with a bitcoin library
  like libbitcoin or cbitcoin. Running a node, even an SPV, is pretty 
  taxing and un-needed, especially on metered connections
 */
#include "main.h"
#include "tx.h"
#include "util.h"
#include "json_rpc.h"
#include "settings.h"
#include "file.h"
#include "lock.h"

// JSON commands, probably isn't needed
#define SEND_ID 1
#define SET_TX_FEE_ID 2
#define WALLETPASSPHRASE_ID 3
#define WALLETLOCK_ID 4


static std::vector<tx_out_t> outputs;
static lock_t outputs_lock;
static std::string tx_from_account;

static int read_all_tx_from_disk(){
	std::vector<std::string> data = newline_to_vector(
		system_handler::cmd_output("find tx | grep '\\.'"));
	for(unsigned int i = 0;i < data.size();i++){
		const std::string tmp = data.at(i);
		bool valid_addr = false;
		bool valid_sat = false;
		bool valid_id = false;
		satoshi_t satoshi = 0;
		std::string address = "";
		unsigned long long int tx_id = 0;
		try{
			address = tmp.substr(0, tmp.find_first_of("."));
			valid_addr = true;
		}catch(...){
			print("unable to get address from transaction'" + tmp + "', not deleting", P_ERR);
			valid_addr = false;
		}
		try{
			tx_id = std::stoul(tmp.substr(tmp.find_first_of(".")+1, tmp.size()));
			valid_id = true;
		}catch(...){
			print("unable to get tx_id from transaction '" + tmp + "', not deleting", P_ERR);
			valid_id = false;
		}
		try{
			satoshi = std::stoul(file::read_file(tmp));
			valid_sat = true;
		}catch(...){
			print("unable to get volume from transaction '" + tmp + "' , not deleting", P_ERR);
			valid_sat = false;
		}
		if(valid_addr && valid_id && valid_sat){
			system_handler::run("rm -r " + tmp);
		}
		if(satoshi != 0 && address != ""){
			tx::add_tx_out(tx_out_t(address, satoshi, tx_id));
		}
	}
	system_handler::run("rm -r output");
	return 0;
}

static satoshi_t tx_outputs_total(){
	satoshi_t retval = 0;
	LOCK_RUN(outputs_lock, [](satoshi_t *retval){
			for(unsigned int i = 0;i < outputs.size();i++){
				*retval += outputs[i].get_satoshi();
			}
		}(&retval));
	return retval;
}

int tx::add_tx_out(tx_out_t transaction){
	LOCK_RUN(outputs_lock,outputs.push_back(transaction));
	return 0;
}

static std::vector<std::pair<std::string, satoshi_t> > tx_send_all_simplify(){
	std::vector<std::pair<std::string, satoshi_t> > output_actual;
	auto function = [&](){
				try{
					for(unsigned int i = 0;i < outputs.size();i++){
						const satoshi_t output_sat =
							outputs[i].get_satoshi();
						const std::string output_addr =
							outputs[i].get_address();
						bool found = false;
						for(unsigned int c = 0;c < output_actual.size();c++){
							const std::string actual_addr =
								output_actual[c].first;
							if(actual_addr == output_addr){
								output_actual[c].second +=
									outputs[i].get_satoshi();
								found = true;
								break;
							}
						}
						if(!found){
							const std::pair<std::string, satoshi_t> actual_pair =
								std::make_pair(output_addr,
									       output_sat);
							output_actual.push_back(
								actual_pair);
						}
					}
				}catch(std::exception e){
					pre_pro::exception(
						e,
						"tx_send_all_simplify"
						, P_ERR);
					throw e;
				}
				outputs.clear();
			};
	try{
		LOCK_RUN(outputs_lock, function());
	}catch(std::exception e){
		pre_pro::exception(e, "tx_send_all_simplify", P_ERR);
		throw e;
	}
	return output_actual;
}

static std::vector<std::pair<std::string, std::vector<std::string> > >
tx_send_all_gen_json_commands(
	std::vector<std::pair<std::string, satoshi_t> > simple){
	std::vector<std::pair<std::string, std::vector<std::string> > > retval;
	std::string wallet_passphrase;
	// unlock the wallet, no specified time limit (yet)
	try{
		wallet_passphrase = settings::get_setting(
			"tx_wallet_passphrase");
	}catch(std::exception e){
		pre_pro::exception(e, "wallet_passphrase", P_ERR);
		throw e;
	}
	try{
		retval.push_back(
			std::make_pair("walletpassphrase",
				       std::vector<std::string>(
				       {wallet_passphrase})));
	}catch(std::exception e){
		pre_pro::exception(e, "tx_wallet_passphrase", P_ERR);
		throw e;
	}
	// sendmany
	try{
		std::string custom_tx_string;
		for(unsigned int i = 0;i < simple.size();i++){
			const satoshi_t val = simple[i].second;
			const long double val_btc = val*get_mul_to_btc("satoshi");
			custom_tx_string +=
				simple[i].first + ":" + std::to_string(val_btc);
			if(i+1 < outputs.size()){
				custom_tx_string += ",";
			}
		}
		custom_tx_string = JSON_BRACES(custom_tx_string);
		retval.push_back(
			std::make_pair("sendmany",
				       std::vector<std::string>(
				       {custom_tx_string})));
	}catch(std::exception e){
		pre_pro::exception(e, "tx_send_all adding txs", P_ERR);
		throw e;
	}
	// lock the wallet again
	retval.push_back(
		std::make_pair("walletlock",
			       std::vector<std::string>({})));
	return retval;
}

static void tx_send_all_json_batch(
	std::vector<std::pair<std::string, std::vector<std::string> > > json_){
	for(unsigned int i = 0;i < json_.size();i++){
		int error_code = 0;
		std::string result;
		json_rpc::cmd(json_[i].first, json_[i].second, i);
		while(json_rpc::resp(&result, &error_code, i) == -1){
			sleep_ms(1000);
			// waiting is important
		}
		if(json_rpc::error_check(error_code) != 0){
			throw std::runtime_error("tx::send_all JSON");
		}
	}

}

int tx::send_all(){
	std::vector<std::pair<std::string, satoshi_t> > simple;
	try{
		simple = tx_send_all_simplify();
	}catch(std::exception e){
		pre_pro::exception(e, "tx_send_all_simplify", P_ERR);
		throw e;
	}
	std::vector<std::pair<std::string, std::vector<std::string> > > json_ =
		tx_send_all_gen_json_commands(simple);
	try{
		//throws runtime_error on JSON API errors
		tx_send_all_json_batch(json_);
	}catch(std::exception e){
		pre_pro::exception(e, "tx_send_all_json_batch", P_ERR);
		throw e;
	}
	/*
	  I probably shouldn't delete txs, but instead record them
	  in a new data type filled with the tx_out_t structures and
	  associate that with a blockchain ID so there is a verifiable
	  and recordable history?
	 */
	LOCK_RUN(outputs_lock, outputs.clear());
	
	// any other errors should be reported through the JSON API
	// move checks for actual run outside of this function, makes
	// overriding them for whatever reason a lot easier
	return 0;
}

int tx::init(){
	read_all_tx_from_disk();
	threads.emplace_back(std::thread([](){
				while(running){
					bool tx_block = false;
					/*
					  Sends out transactions in large-ish chunks, should be
					 */
					try{
						tx_block |= outputs.size() >= (unsigned int)std::stoul(settings::get_setting("tx_outputs_until_block"));
					}catch(std::invalid_argument e){
						print("invalid argument for outputs_until_block", P_ERR);
					}catch(std::out_of_range e){
						print("out of range for outputs_until_block", P_ERR);
					}
					try{
						tx_block |= tx_outputs_total() >= (unsigned int)std::stoul(settings::get_setting("tx_volume_until_block"));
					}catch(std::invalid_argument e){
						print("invalid argument for volume_until_block", P_ERR);
					}catch(std::out_of_range e){
						print("out of range for volume_until_block", P_ERR);
					}
					if(tx_block){
						tx::send_all();
					}
					sleep_ms(DEFAULT_THREAD_SLEEP);
				}
			}));
	return 0;
}

int tx::close(){
	return 0;
}

void tx_out_t::set_address(std::string address_){
	address = address_;
	update_file();
}

std::string tx_out_t::get_address(){
	return address;
}

void tx_out_t::set_satoshi(satoshi_t satoshi_){
	satoshi = satoshi_;
	update_file();
}

satoshi_t tx_out_t::get_satoshi(){
	return satoshi;
}

std::string tx_out_t::get_filename(){
	return "tx/" + address + "." + std::to_string(tx_id);
}

void tx_out_t::update_file(){
	file::write_file(get_filename(), std::to_string(satoshi));
}

tx_out_t::tx_out_t(std::string address_, satoshi_t satoshi_, unsigned long long int tx_id){
	address = address_;
	satoshi = satoshi_;
	if(tx_id == 0){
		while(tx_id == 0 && file::exists(get_filename())){
			tx_id = std::rand();
		}
	}
	update_file();
}

tx_out_t::~tx_out_t(){
	system_handler::run("rm -r " + get_filename());
	satoshi = 0;
	address = "";
}
