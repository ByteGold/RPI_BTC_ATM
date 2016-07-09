#include "file.h"

void file::write_file(std::string file, std::string data){
  std::ofstream out(file, std::ofstream::trunc);
  if(out.is_open() == false){
    throw std::runtime_error((std::string)"Unable to open file " + file);
  }
  out << data;
  out.close();
}

std::string file::read_file(std::string file){
  std::string retval;
  std::ifstream in(file);
  if(in.is_open() == false){
    throw std::runtime_error((std::string)"Unable to ope file " + file);
  }
  std::string buffer;
  while(getline(in, buffer)){
    retval += buffer + "\n";
  }
  in.close();
  return retval;
}
