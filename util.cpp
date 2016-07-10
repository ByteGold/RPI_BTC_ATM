#include "main.h"
#include "util.h"

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
}

std::string get_argv(int a){
  if(a >= argc){
    return argv[a];
  }else{
    throw std::runtime_error("get_argv went out of bounds");
  }
}

std::string get_argv_val(std::string a){
  return get_argv(search_for_argv(a));
}
