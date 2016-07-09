#include "main.h"
#include "util.h"

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
}

void print(std::string data, int level){
  std::cout << print_level_text(level) << " " << data << std::endl;
}
