#ifndef QR_H
#define QR_H
#include "cstdlib"
namespace qr{
  void gen_qr_code(std::string str, std::string file);
  std::string read_qr_code();
}
#endif
