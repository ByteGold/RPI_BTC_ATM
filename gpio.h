#ifndef GPIO_H
#define GPIO_H
#include "iostream"
#include "cstdlib"
#include "string"
#include "exception"
#include "stdexcept"
#include "vector"
#include "mutex"
#include "thread"
#define GPIO_IN 1
#define GPIO_OUT 0
struct gpio_pin_t{
private:
  int pin;
  int power;
  int dir;
  int blink;
public:
  gpio_pin_t();
  ~gpio_pin_t();
  void set_pin(int);
  int get_pin();
  void set_power(int);
  int get_power();
  void set_dir(int);
  int get_dir();
  void set_blink(int);
  int get_blink();
};
namespace gpio{
	char get_val(int pin);
	char set_val(int pin, bool val);
	char get_dir(int pin);
	char set_dir(int pin, int dir);
	char init();
}

#endif
