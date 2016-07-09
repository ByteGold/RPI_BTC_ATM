#include "gpio.h"
#include "file.h"
#include "util.h"

static int gpio_count = 0;

#define DISABLED_GPIO() if(search_for_argv("--no-gpio") != -1) return 0;

static std::string gen_gpio_val_file(int pin){
  return (std::string)"/sys/class/gpio/gpio" + std::to_string(pin) + (std::string)"/value";
}

static std::string gen_gpio_dir_file(int pin){
  return (std::string)"/sys/class/gpio/gpio" + std::to_string(pin) + (std::string)"/direction";
}

char gpio::get_val(int pin){
  DISABLED_GPIO();
  return file::read_file(gen_gpio_val_file(pin))[0];
}

char gpio::set_val(int pin, bool val){
  DISABLED_GPIO();
  file::write_file(gen_gpio_val_file(pin), std::string({val, 0}));
  return 0;
}

char gpio::get_dir(int pin){
  DISABLED_GPIO();
  return file::read_file(gen_gpio_dir_file(pin))[0];
}

char gpio::set_dir(int pin, int dir){
  DISABLED_GPIO();
  file::write_file(gen_gpio_dir_file(pin), ((dir == GPIO_IN) ? "in" : "out"));
  return 0;
}

char gpio::init(){
  DISABLED_GPIO();
  try{
    while(true){
      gpio::get_val(gpio_count);
    }
    gpio_count++;
  }catch(...){}
  switch(gpio_count){
  case 0:
    throw std::runtime_error("No GPIO pins detected, terminating program");
  case 26:
    print("Using Raspberry Pi Model B (26 GPIO pins)", PRINT_NOTICE);
    break;
  case 40:
    print("Using Raspberry Pi Model A/B+/Zero/2 (40 GPIO pins)", PRINT_NOTICE);
    break;
  default:
    print("Using something that isn't a Raspberry Pi", PRINT_NOTICE);
    break;
  }
  return 0;
}
