#include "main.h"
#include "util.h"
#include "file.h"

int print_level = -1;

int search_for_argv(std::string value){
  for(int i = 0;i < argc;i++){
    if(std::strcmp(argv[i], value.c_str()) == 0){
      return i;
    }
  }
  return -1;
}

static std::string print_level_text(int level){
  switch(level){
  case P_DEBUG:
    return "[DEBUG]";
  case P_NOTICE:
    return "[NOTICE]";
  case P_WARN:
    return "[WARN]";
  case P_ERR:
    return "[ERROR]";
  case P_CRIT:
    return "[CRITICAL]";
  }
  throw std::runtime_error("invalid print level");
}

void print(std::string data, int level){
  if(print_level == -1){
    if(search_for_argv("--debug") != -1){
      print_level = P_DEBUG;
    }else{
      print_level = P_NOTICE; // sane minimum
    }
  }
  if(level >= print_level){
    std::cout << print_level_text(level) << " " << data << std::endl;
  }
  if(level == P_CRIT){
    throw std::runtime_error(data);
  }
}

long double get_btc_rate(std::string currency){
  // 3 character code
  if(currency == "USD" || currency == "usd"){
    system("wget https://blockchain.info/q/24hrprice");
    long double price = std::stold(file::read_file("24hrprice"));
    print("the price is " + std::to_string(price) + " per BTC", P_NOTICE);
    return price;
  }else{
    print("your plebian currency isn't supported yet", P_CRIT);
    return 0; // complains about no retval
  }
}

int system_(std::string str){
  print("system: " + str, P_DEBUG);
  /*
    Most commands need some time to be processed on the lower level (GPIO).
    Speed shouldn't be a problem
   */
  int retval = system(str.c_str());
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  return retval;
}
