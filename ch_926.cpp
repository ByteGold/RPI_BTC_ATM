/*
This is the driver for the CH-926 coin acceptor. This was chosen because it was
on adafruit.com and Alibaba has a fair amount of vendors that sell this version
of acceptor.

For the sake of debugging, this is assuming the output signal is set to 100ms
The supplied table is the default value, and accepts pennies, nickles, dimes,
and quarters (half dollars and dollar coins shouldn't be used to buy things anyways)
 */
#include "main.h"
#include "ch_926.h"
#include "util.h"
#include "gpio.h"

#define CH_926_PULSE_TIME 100

#define CH_926_GPIO_IN 0
#define CH_926_GPIO_POWER 1

static int gpio_pins[2];
static std::array<int, 5> value_table = {0, 1, 5, 10, 25};

//sane default value, no need for bounds checking because the acceptor shouldn't
//return a pulse set it wasn't specifically programmed for, and the proper table should be set

int ch_926_init(){
  gpio_pins[CH_926_GPIO_IN] = 1;
  gpio_pins[CH_926_GPIO_POWER] = 2;
  gpio::set_dir(gpio_pins[CH_926_GPIO_POWER], GPIO_OUT); // should be enough
  gpio::set_dir(gpio_pins[CH_926_GPIO_IN], GPIO_IN);
  //TODO: add other currencies
  return 0;
}

int ch_926_close(){
  //possibly reset the GPIO pins to input?
  return 0;
}

int ch_926_run(int *count){
  unsigned long int pulse_count = 0;
  if(search_for_argv("--ch-926-debug") != -1){
    std::cout << "pulse count:";
    std::cin >> pulse_count;
  }else if(gpio::get_val(gpio_pins[CH_926_GPIO_IN]) != 0){
    pulse_count++;
    std::this_thread::sleep_for(std::chrono::milliseconds(CH_926_PULSE_TIME));
    while(gpio::get_val(gpio_pins[CH_926_GPIO_IN]) != 0){
      pulse_count++;
      std::this_thread::sleep_for(std::chrono::milliseconds(CH_926_PULSE_TIME));
    }
  }
  *count += value_table[pulse_count];
  return 0;
}
