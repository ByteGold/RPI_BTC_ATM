#ifndef DRIVER_H
#define DRIVER_H
#include "functional"
#include "mutex"
#include "lock.h"
class driver_t{
public:
	uint64_t count;
	lock_t lock;
	std::function<int()> init;
	std::function<int()> close;
	std::function<int(uint64_t*)> run;
	std::function<int()> reject_all;
	std::function<int()> accept_all;
	driver_t();
	~driver_t();
	uint64_t get_count();
	void set_count(uint64_t);
};
#endif
