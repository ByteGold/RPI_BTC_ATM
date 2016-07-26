#include "tx.h"
#ifndef DEPOSIT_H
#define DEPOSIT_H
#define DEPOSIT_SEND_DEPOSIT 0 // standard deposit, take out fees
struct fee_t{
private:
	std::string address;
	long double percent;
public:
	fee_t(std::string address_, long double percent_);
	~fee_t();
	void set_address(std::string address_);
	std::string get_address();
	void set_percent(long double percent_);
	long double get_percent();
};
namespace deposit{
	void add_fee(fee_t fee_);
	int send(std::string address, satoshi_t satoshi, int type = DEPOSIT_SEND_DEPOSIT);  
	int init();
	int close();
}
#endif
