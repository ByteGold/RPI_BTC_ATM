#ifndef SETTINGS_H
#define SETTINGS_H
#include "vector"
#include "utility"
#include "string"
#include "mutex"
#include "sstream"
namespace settings{
  void set_settings();
  std::string get_setting(std::string);
}
#endif
