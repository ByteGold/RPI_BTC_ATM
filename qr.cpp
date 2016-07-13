#include "main.h"
#include "util.h"
#include "qr.h"

void qr::gen_qr_code(std::string str, std::string file){
  system(("echo \""+ str +"\" | qr > " + file).c_str());
  //error checking and compression?
}

std::string qr::read_qr_code(){
  return "";
}
