#include "main.h"
#include "tx.h"
#include "util.h"
#include "json_rpc.h"
#include "settings.h"

#define SEND_ID 1
#define SET_TX_FEE_ID 2
#define WALLETPASSPHRASE_ID 3
#define WALLETLOCK_ID 4

static std::vector<tx_out_t> outputs;
static std::mutex outputs_lock;
static std::string tx_from_account;
static std::thread send_transaction_block_thread;

static satoshi_t tx_outputs_total(){
  satoshi_t retval = 0;
  LOCK_RUN(outputs_lock, [](satoshi_t *retval){
      for(unsigned int i = 0;i < outputs.size();i++){
	retval += outputs[i].get_satoshi();
      }
    }(&retval));
  return retval;
}

int tx::add_tx_out(tx_out_t transaction){
  LOCK_RUN(outputs_lock,outputs.push_back(transaction));
  return 0;
}

static int auto_set_tx_fee(){
  const int fee_size = (75*50)*(outputs.size()+2);
  json_rpc::cmd("settxfee", {std::to_string(fee_size)}, SET_TX_FEE_ID);
  return fee_size;
}

int tx::send_transaction_block(){
  // TODO: unlock the wallet for a second or two
  json_rpc::cmd("walletpassphrase", {settings::get_setting("passphrase"), "10"}, WALLETPASSPHRASE_ID); // is 10 too high?
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
  json_rpc::cmd("walletlock", {}, WALLETLOCK_ID);
  return 0;
}

int tx::init(){
  send_transaction_block_thread = std::thread([](){
    while(true){
      if(outputs.size() >= (unsigned int)std::stoul(settings::get_setting("outputs_until_block"))){
	tx::send_transaction_block();
      }
    }
    });
  return 0;
}

int tx::close(){
  send_transaction_block_thread.join();
  return 0;
}

/*
  TODO: save transactions to disk
 */

void tx_out_t::set_address(std::string address_){
  address = address_;
}

std::string tx_out_t::get_address(){
  return address;
}

void tx_out_t::set_satoshi(satoshi_t satoshi_){
  satoshi = satoshi_;
}

satoshi_t tx_out_t::get_satoshi(){
  return satoshi;
}

tx_out_t::tx_out_t(std::string address_, satoshi_t satoshi_){
  address = address_;
  satoshi = satoshi_;
}

tx_out_t::~tx_out_t(){
  satoshi = 0;
}
