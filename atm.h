#ifndef ATM_H
#define ATM_H
#include "tx.h" // satoshi_t
namespace atm{
	void init();
	void close();
	satoshi_t loop();
};

#endif
