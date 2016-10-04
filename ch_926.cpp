/*
This is the driver for the CH-926 coin acceptor. This was chosen because it was
on adafruit.com and Alibaba has a fair amount of vendors that sell this version
of acceptor.

Although the file says CH-926, this can be used with any CH-92X series acceptor, 
but the built-in table (value_table_usd) is programmed to use the CH-926 acceptor, 
look at the settings.cfg file for how to set custom pulses in settings.cfg
The supplied table should work for the CH-924 or higher automatically.

For the sake of debugging, this is assuming the output signal is set to 100ms
The supplied table is the default value, and accepts pennies, nickles, dimes,
and quarters
 */
#include "main.h"
#include "ch_926.h"
#include "util.h"
#include "gpio.h"
#include "settings.h"

#define CH_926_PULSE_TIME 100

#define CH_926_GPIO_IN 0
#define CH_926_GPIO_POWER 1

static int gpio_pins[2];

static std::array<int, 7> value_table_usd = {0, 1, 5, 10, 25, 0, 0};
static std::array<int, 7> *value_table = nullptr;

//sane default value, no need for bounds checking because the acceptor shouldn't
//return a pulse set it wasn't specifically programmed for, and the proper table should be set

int ch_926_init(){
	gpio_pins[CH_926_GPIO_IN] = 1;
	gpio_pins[CH_926_GPIO_POWER] = 2;
	gpio::set_dir(gpio_pins[CH_926_GPIO_POWER], GPIO_OUT); // should be enough
	gpio::set_dir(gpio_pins[CH_926_GPIO_IN], GPIO_IN);
	if(settings::get_setting("currency") == "USD"){
		print("setting currency to USD", P_NOTICE);
		value_table = &value_table_usd;
		try{
			if(settings::get_setting("ch_926_1_pulse") != ""){
				(*value_table)[1] = std::stoul(settings::get_setting("ch_926_1_pulse"));
			}
			if(settings::get_setting("ch_926_2_pulse") != ""){
				(*value_table)[2] = std::stoul(settings::get_setting("ch_926_2_pulse"));
			}
			if(settings::get_setting("ch_926_3_pulse") != ""){
				(*value_table)[3] = std::stoul(settings::get_setting("ch_926_3_pulse"));
			}
			if(settings::get_setting("ch_926_4_pulse") != ""){
				(*value_table)[4] = std::stoul(settings::get_setting("ch_926_4_pulse"));
			}
			if(settings::get_setting("ch_926_5_pulse") != ""){
				(*value_table)[5] = std::stoul(settings::get_setting("ch_926_5_pulse"));
			}
			if(settings::get_setting("ch_926_6_pulse") != ""){
				(*value_table)[6] = std::stoul(settings::get_setting("ch_926_6_pulse"));
			}
		}catch(std::invalid_argument e){
			print("invalid argument for ch 926 pulse count", P_ERR);
			throw e;
		}catch(std::out_of_range e){
			print("currency is out of range for ch 926 pulse count", P_ERR);
			throw e;
		}
	}else{
		print("price ticker has no support for your non-USD currency", P_CRIT);
		// TODO: fix this	
	}
	return 0;
}

int ch_926_close(){
	//possibly reset the GPIO pins to input?
	return 0;
}

/*
  If the pulses don't match up properly, make sure it has been grounded well
*/

int ch_926_run(int *count){
	long int pulse_count = 0;
	if(settings::get_setting("ch_926_debug") == "true"){
		std::cout << "pulse count:";
		std::string pulse_count_input; // protection against absurd values
		std::cin >> pulse_count_input;
		try{
			pulse_count = std::stol(pulse_count_input);
		}catch(std::exception e){
			pre_pro::exception(e, "pulse_count_input", P_ERR);
			pulse_count = 0;
		}
	}else if(gpio::get_val(gpio_pins[CH_926_GPIO_IN]) != 0){
		pulse_count++;
		sleep_ms(CH_926_PULSE_TIME);
		while(gpio::get_val(gpio_pins[CH_926_GPIO_IN]) != 0){
			pulse_count++;
			sleep_ms(CH_926_PULSE_TIME);
		}
	}
	if(pulse_count < 0){
		print("pulse count is negative, something messed up badly", P_ERR);
		return -1;
	}
	if(pulse_count == 0){
		print("pulse count is zero, quitting early", P_SPAM);
		return 0;
	}
	try{
		*count += value_table->at(pulse_count);
		print("added " + std::to_string(value_table->at(pulse_count)) +" to count", P_NOTICE);
	}catch(std::exception e){
		pre_pro::exception(e, "pulse_count", P_ERR);
	}
	return 0;
}

/*
  I heard that this feature exists, and it would be awful cool, but I can't 
  find any documentation on it anywhere. I need the physical unit for this,
  I hope this isn't a DIP switch I have to mess around with in GPIO.
*/


int ch_926_accept_all(){
	return -1;
}

int ch_926_reject_all(){
	return -1;
}
