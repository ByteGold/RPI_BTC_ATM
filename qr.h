#ifndef QR_H
#define QR_H
#include "cstdlib"
namespace qr{
	int init();
	int close();
	int gen(std::string str, std::string file);
	std::string read(std::string file);
	std::string read_from_webcam();
}
#endif
