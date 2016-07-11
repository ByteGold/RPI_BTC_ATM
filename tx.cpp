#include "main.h"
#include "tx.h"
#include "util.h"
#include "json_rpc.h"

#define SEND_ID 1
#define SET_TX_FEE_ID 2

static std::vector<tx_out_t> outputs;
static std::mutex outputs_lock;
static std::string tx_from_account;

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

int tx::send_transaction_log(){
  // TODO: unlock the wallet for a second or two
  set_tx_fee(1000);
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
      print("tx backlog exists, waiting until it is profitable to send", P_DEBUG);
    }
  }
  return 0;
}

int tx::set_tx_fee(satoshi_t satoshi){
  json_rpc::cmd("settxfee", {std::to_string(satoshi)}, SET_TX_FEE_ID);
  return 0;
}

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

tx_out_t::tx_out_t(){
  satoshi = 0;
}

tx_out_t::~tx_out_t(){
  satoshi = 0;
}
