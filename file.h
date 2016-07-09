#ifndef FILE_H
#define FILE_H
#include "fstream"
#include "exception"
#include "stdexcept"
namespace file{
	void write_file(std::string file, std::string data);
	std::string read_file(std::string file);
};
#endif
