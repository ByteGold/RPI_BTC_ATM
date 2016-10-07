#ifndef TX_H
#define TX_H

#include "mutex"
#include "string"

typedef uint64_t satoshi_t;
typedef long double btc_t;

/*
  TODO: implement the index system for more secure storage of private
  keys. RAM usage shouldn't be a big problem, so 1024 layers or so should
  suffice. 
 */

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

namespace tx{
	int init();
	int close();
	int send_all();
	int add_tx_out(tx_out_t transaction);
};
#endif
