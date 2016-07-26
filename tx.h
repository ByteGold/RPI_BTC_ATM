#ifndef TX_H
#define TX_H

#include "mutex"
#include "string"

#define TX_SIMPLE_FEE 5500 // std. fee for a one output transaction

typedef uint64_t satoshi_t;
typedef long double btc_t;

struct tx_out_t{
private:
  std::string address;
  satoshi_t satoshi;
  unsigned long long int tx_id; // no connection to bitcoin network id
public:
  tx_out_t(std::string address_, satoshi_t satoshi_, unsigned long long int tx_id = 0);
  ~tx_out_t();
  void set_address(std::string);
  std::string get_address();
  void set_satoshi(satoshi_t);
  satoshi_t get_satoshi();
  std::string get_filename(); // generated from address & tx_id
  void update_file();
};

/*
  NOTE: The JSON-RPC uses BTC as the unit of Bitcoin, not Satoshi. However, to
  prevent any rounding errors, satoshi will be used inside of the program until
  it is sent to the JSON-RPC API via json_rpc::send_cmd
  
  Using an int32_t for Satoshi caps the value of the transaction at ~20BTC, or $13,912
  at $650. If that much money goes through this machine, i'd be suprised. int64_t
  sets the max at ~60 trillion dollars, so we should be good on that front, and int128_t
  should be good enough until the end of time
*/

namespace tx{
  int init();
  int close();
  int send_transaction_block();
  int add_tx_out(tx_out_t transaction);
};
#endif
