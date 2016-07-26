#include "main.h"
#include "util.h"
#include "qr.h"
#include "file.h"
#include "settings.h"

#define DISABLED_QR_STR() if(settings::get_setting("no_qr") == "true"){return "";}
#define DISABLED_QR_INT() if(settings::get_setting("no_qr") == "true"){return 0;}
#define DISABLED_QR_VOID() if(settings::get_setting("no_qr") == "true"){return;}

static std::string qr_data = "";
static std::mutex qr_data_lock;
static FILE* qr_reader_file_desc = NULL;
static int qr_run_number_of_lines = 0;

static void qr_run(){
	DISABLED_QR_VOID();
	char input[100];
	std::string qr_data_tmp;
	long int line_cnt = 0;
	while(running){
		while(!feof(qr_reader_file_desc)){
			line_cnt++;
			if(fgets(input, 100, qr_reader_file_desc) && qr_run_number_of_lines != line_cnt){
				print("reading data qr_run", P_DEBUG);
				qr_run_number_of_lines = line_cnt;
				line_cnt = 0;
				for(int i = 0;i < 100;i++){
					if(input[i] == '\n'){
						input[i] = ' ';
					}
				}
				LOCK_RUN(qr_data_lock, [](char *input){
						std::stringstream ss(input);
						ss >> qr_data;
						memset(input, 0, 100);
						print("scanned new qr code '" + qr_data + "'", P_NOTICE);
					}(input));
			}else{
				print("no new info from qr_run", P_SPAM);
			}
		}
		print("rewinding qr_run file descriptor", P_SPAM);
		rewind(qr_reader_file_desc);
		sleep_ms(DEFAULT_THREAD_SLEEP);
	}
}

int qr::init(){
	DISABLED_QR_INT();
	if(file::exists("/dev/video0") == false){
		print("no camera detected for qr module, disable with no_qr", P_CRIT);
	}
	qr_reader_file_desc = popen("zbarcam --nodisplay --raw", "r");
	threads.emplace_back(std::thread(qr_run));
	return 0;
}

// TODO: make or get a legit library for more flexibility

int qr::gen(std::string str, std::string file){
	DISABLED_QR_INT();
	system_(("echo \""+ str +"\" | qr > " + file).c_str());
	return 0;
}

std::string qr::read(std::string file){
	DISABLED_QR_STR();
	std::string retval = file::read_file(file);
	return retval;
}

std::string qr::read_from_webcam(){
	DISABLED_QR_STR();
	std::string retval;
	LOCK_RUN(qr_data_lock, [](std::string *retval){
			*retval = qr_data;
			print("read qr code " + qr_data + " from qr_data", P_NOTICE);
			qr_data = "";
		}(&retval));
	qr_data = "";
	return retval;
}

int qr::close(){
	DISABLED_QR_INT();
	if(settings::get_setting("no_qr") == "false"){
		return -1;
	}
	system("ps aux | grep -i zbarcam | awk {'print $2'} | xargs kill -9");
	// it works, 	but I need to remake system()
	pclose(qr_reader_file_desc);
	return 0;
}
