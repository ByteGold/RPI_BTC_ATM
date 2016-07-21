#include "main.h"
#include "settings.h"
#include "file.h"
#include "util.h"

static std::vector<std::pair<std::string, std::string> > settings_vector;
static std::mutex settings_lock;

void settings::set_settings(){
  std::string cfg_file = file::read_file("settings.cfg");
  std::stringstream ss(cfg_file);
  std::string temp_str;
  char setting[512];
  char var[512];
  while(std::getline(ss, temp_str)){
    if(temp_str[0] == '#'){
      continue;
    }
    memset(setting, 0, 512);
    memset(var, 0, 512);
    if(std::sscanf(temp_str.c_str(), "%s = %s", setting, var) == EOF){
      print("setting has no variable or otherwise indecipherable", P_ERR);
      continue; // no variable or otherwise indecipherable
    }
    print("setting " + (std::string)setting + " == " + (std::string)var, P_DEBUG);
    LOCK_RUN(settings_lock, [](char *setting, char *var){
	settings_vector.push_back(std::make_pair(setting, var));
      }(setting, var));
  }
}

// no real way to know the type, so leave that to the parent
std::string settings::get_setting(std::string setting){
  std::string retval;
  print("requesting setting " + setting, P_DEBUG);
  LOCK_RUN(settings_lock, [](std::string *retval, std::string *setting){
      for(unsigned int i = 0;i < settings_vector.size();i++){
	if(settings_vector[i].first == *setting){
	  *retval = settings_vector[i].second;
        }
      }
    }(&retval, &setting));
  return retval;
}
