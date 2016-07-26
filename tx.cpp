#include "main.h"
#include "tx.h"
#include "util.h"
#include "json_rpc.h"
#include "settings.h"
#include "file.h"

#define SEND_ID 1
#define SET_TX_FEE_ID 2
#define WALLETPASSPHRASE_ID 3
#define WALLETLOCK_ID 4

static std::vector<tx_out_t> outputs;
static std::mutex outputs_lock;
static std::string tx_from_account;

static int read_all_tx_from_disk(){
  system_write("find tx | grep '\\.'", "output");
  std::stringstream ss(file::read_file("output"));
  std::string tmp;
  while(std::getline(ss, tmp)){
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
      system_("rm -r " + tmp);
    }
    if(satoshi != 0 && address != ""){
      tx::add_tx_out(tx_out_t(address, satoshi, tx_id));
    }
  }
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

static int auto_set_tx_fee(){
  int fee_size = 0;
  if(settings::get_setting("fixed_fee") == ""){
    fee_size = (75*50)*(outputs.size()+2);
  }else{
    try{
      fee_size = std::stoi(settings::get_setting("fixed_fee"));
    }catch(std::invalid_argument e){
      print("invalid argument for fixed_fee", P_ERR);
    }catch(std::out_of_range e){
      print("out of range for fixed_fee", P_ERR);
    }
  }
  json_rpc::cmd("settxfee", {std::to_string(fee_size)}, SET_TX_FEE_ID);
  return fee_size;
}

int tx::send_transaction_block(){
  // TODO: unlock the wallet for a second or two
  if(settings::get_setting("tx_wallet_passphrase") != ""){
    json_rpc::cmd("walletpassphrase", {settings::get_setting("tx_wallet_passphrase"), "10"}, WALLETPASSPHRASE_ID);
    // 10 isn't too high assuming walletlock runs
  }
  try{
    int fee = auto_set_tx_fee();
    print("transaction fee is " + std::to_string(fee), P_NOTICE);
    if((fee/100000000)*get_btc_rate("USD") > .50){
      print("transaction fee is too high, setting sane setting, force with --force-fee", P_ERR);
      fee = .25/get_btc_rate("USD");
    }
    if(outputs.size() == 1){
      json_rpc::cmd("sendtoaddress", {outputs[0].get_address(), std::to_string(outputs[0].get_satoshi()/100000000.0)}, SEND_ID);
      outputs.erase(outputs.begin()+0);
    }else{
      if(tx_outputs_total() >= 100000){
	std::string custom_tx_string;
	LOCK_RUN(outputs_lock,{
	    for(unsigned int i = 0;i < outputs.size();i++){
	      custom_tx_string += outputs[i].get_address() + ":" + std::to_string(outputs[i].get_satoshi()/100000000.0);
	      if(i+1 < outputs.size()){
		custom_tx_string += ",";
	      }
	    }
	    outputs.clear();
	  });
	custom_tx_string = JSON_BRACES(custom_tx_string);
	json_rpc::cmd("sendmany", {tx_from_account, custom_tx_string}, SEND_ID);
      }else{
	print("not enough money transacted in block to warrant transmitting, waiting", P_DEBUG);
      }
    }
  }catch(...){
    print("unable to send transaction block, LOOK INTO THIS AND MAKE PROPER EXCEPTIONS", P_ERR);
    json_rpc::cmd("walletlock", {}, WALLETLOCK_ID);
  }
  return 0;
}

int tx::init(){
  read_all_tx_from_disk();
  threads.emplace_back(std::thread([](){
    while(running){
      bool tx_block = false;
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
	tx::send_transaction_block();
      }
      sleep_ms(DEFAULT_THREAD_SLEEP);
    }
      }));
  return 0;
}

int tx::close(){
  return 0;
}

/*
  TODO: save transactions to disk
 */

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
  system_("rm -r " + get_filename());
  satoshi = 0;
  address = "";
}
