#include "gpio.h"
#include "file.h"
#include "util.h"
#include "main.h"
#include "settings.h"
#include "lock.h"

#define DISABLED_GPIO() if(settings::get_setting("no_gpio") == "true") return 0;

static int gpio_count = 0;
static std::vector<gpio_pin_t> gpio_pins;
static lock_t gpio_pins_lock;

static std::string gen_gpio_val_file(int pin){
	if(pin > gpio_count || pin < 0){
		throw std::runtime_error("gpio pin invalid");
	}
	return (std::string)"/sys/class/gpio/gpio" + std::to_string(pin) + (std::string)"/value";
}

static std::string gen_gpio_dir_file(int pin){
	if(pin > gpio_count || pin < 0){
		throw std::runtime_error("gpio pin invalid");
	}
	return (std::string)"/sys/class/gpio/gpio" + std::to_string(pin) + (std::string)"/direction";
}

char gpio::get_val(int pin){
	DISABLED_GPIO();
	char retval = 127;
	LOCK_RUN(gpio_pins_lock, [](int pin, char *retval){
			for(unsigned int i = 0;i < gpio_pins.size();i++){
				if(gpio_pins[i].get_pin() == pin){
					*retval = file::read_file(gen_gpio_val_file(pin))[0];
					return;
				}
			}
		}(pin, &retval));
	if(retval == 127){
		gpio::add_pin(pin);
		retval = 0;
	}
	return retval;
}

char gpio::set_val(int pin, bool val){
	DISABLED_GPIO();
	LOCK_RUN(gpio_pins_lock, [](int pin, bool val){
			for(unsigned int i = 0;i < gpio_pins.size();i++){
				if(gpio_pins[i].get_pin() == pin){
					file::write_file(gen_gpio_val_file(pin), ((val) ? "1" : "0"));
					return;
				}
			}
		}(pin, val));
	return 0;
}

char gpio::get_dir(int pin){
	DISABLED_GPIO();
	char retval = 0;
	LOCK_RUN(gpio_pins_lock, [](int pin, char *retval){
			for(unsigned int i = 0;i < gpio_pins.size();i++){
				if(gpio_pins[i].get_pin() == pin){
					*retval = file::read_file(gen_gpio_dir_file(pin))[0];
					return;
				}
			}
    }(pin, &retval));
	return retval;
}

char gpio::set_dir(int pin, int dir){
	DISABLED_GPIO();
	LOCK_RUN(gpio_pins_lock, [](int pin, bool dir){
			for(unsigned int i = 0;i < gpio_pins.size();i++){
				if(gpio_pins[i].get_pin() == pin){
					file::write_file(gen_gpio_dir_file(pin), ((dir) ? "1" : "0"));
					return;
				}
			}
		}(pin, dir));
	return 0;
}

int gpio::init(){
	DISABLED_GPIO();
	gpio_count = 26; // forced minimum, ngpio isn't reliable for RPi
	switch(gpio_count){
	case 0:
		throw std::runtime_error("No GPIO pins detected, terminating program");
	case 26-1:
		print("Using Raspberry Pi Model A/B (26 GPIO pins)", PRINT_NOTICE);
		break;
	case 40-1:
		print("Using Raspberry Pi Model A+/B+/Zero/2 (40 GPIO pins)", PRINT_NOTICE);
		break;
	default:
		print("Using something that isn't a Raspberry Pi", PRINT_NOTICE);
		break;
	}
	threads.emplace_back(std::thread([](){
				while(running){
					LOCK_RUN(gpio_pins_lock,{
							for(unsigned int i = 0;i < gpio_pins.size();i++){
								const int pin = gpio_pins[i].get_pin();
								const int power = gpio_pins[i].get_power();
								const int dir = gpio_pins[i].get_dir();
								const int blink = gpio_pins[i].get_blink();
								file::write_file(gen_gpio_dir_file(pin), ((dir == GPIO_IN) ? "in" : "out"));
								if(blink == 0){
									file::write_file(gen_gpio_val_file(pin), std::string({(char)power, 0}));
								}else{
								}
							}
						});
					sleep_ms(DEFAULT_THREAD_SLEEP);
				}
			}));
	return 0;
}

int gpio::close(){
	return 0;
}

void gpio::add_pin(int pin){
	LOCK_RUN(gpio_pins_lock, [](int pin){
			int exist = 0;
			for(unsigned int i = 0;i < gpio_pins.size();i++){
				if(gpio_pins[i].get_pin() == pin){
					print("cannot add pin that is already in use", P_CRIT);
					exist = 1;
					break;
				}
			}
			if(exist != 1){
				print("adding pin " + std::to_string(pin), P_DEBUG);
				gpio_pin_t pin_data(pin);
				gpio_pins.push_back(pin_data);
			}
		}(pin));
}

void gpio::del_pin(int pin){
	LOCK_RUN(gpio_pins_lock, [](int pin){
			for(unsigned int i = 0;i < gpio_pins.size();i++){
				if(gpio_pins[i].get_pin() == pin){
					gpio_pins.erase(gpio_pins.begin()+i);
					break;
				}
			}
		}(pin));  
}

void gpio_pin_t::export_current_pin(){
	file::write("/sys/class/gpio/export", std::to_string(pin));
}

void gpio_pin_t::unexport_current_pin(){
	file::write("/sys/class/gpio/unexport", std::to_string(pin));
}

void gpio_pin_t::set_pin(int pin_){
	if(file::exists("/sys/class/gpio/gpio"+std::to_string(pin))){
		unexport_current_pin();
	}
	pin = pin_;
	export_current_pin();
}

int gpio_pin_t::get_pin(){
	return pin;
}

void gpio_pin_t::set_power(int power_){
	power = power_;
}

int gpio_pin_t::get_power(){
	return power;
}

void gpio_pin_t::set_dir(int dir_){
	dir = dir_;
}

int gpio_pin_t::get_dir(){
	return dir;
}

void gpio_pin_t::set_blink(int blink_){
	blink = blink_;
}

int gpio_pin_t::get_blink(){
	return blink;
}

gpio_pin_t::gpio_pin_t(int pin_){
	set_pin(pin_);
	power = 0;
	dir = 0;
	blink = 0;
}

gpio_pin_t::~gpio_pin_t(){
	unexport_current_pin();
}
