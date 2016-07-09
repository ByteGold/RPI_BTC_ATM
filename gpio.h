#ifndef GPIO_H
#define GPIO_H
#include "iostream"
#include "cstdlib"
#include "string"
#include "exception"
#include "stdexcept"
#define GPIO_IN 1
#define GPIO_OUT 0
namespace gpio{
	char get_val(int pin);
	char set_val(int pin, bool val);
	char get_dir(int pin);
	char set_dir(int pin, int dir);
	char init();
}

#endif
