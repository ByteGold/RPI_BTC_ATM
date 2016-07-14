#include "main.h"
#include "util.h"
#include "qr.h"
#include "file.h"

static std::string qr_data;
static std::thread qr_thread;
static FILE* qr_reader_file_desc = NULL;

static void qr_run(){
  char input[100];
  while(running){
    while(!feof(qr_reader_file_desc)){
      if(fgets(input, 100, qr_reader_file_desc)){
	qr_data = std::string(input);
	memset(input, 0, 100);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_THREAD_SLEEP));
  }
}

int qr::init(){
  qr_reader_file_desc = popen("zbarcam --nodisplay --raw", "r");
  qr_thread = std::thread(qr_run);
  return 0;
}

// TODO: make or get a legit library for more flexibility

int qr::gen(std::string str, std::string file){
  system_(("echo \""+ str +"\" | qr > " + file).c_str());
  return 0;
}

std::string qr::read(std::string file){
  std::string retval = file::read_file(file);
  return retval;
}

int qr::close(){
  system_("pkill -9 zbarcam");
  pclose(qr_reader_file_desc);
  return 0;
}
