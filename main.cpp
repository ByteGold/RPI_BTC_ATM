#include "main.h"
#include "util.h"
#include "file.h"
#include "gpio.h"
#include "driver.h"
#include "ch_926.h"
#include "json_rpc.h"
#include "qr.h"
#include "deposit.h"
#include "settings.h"
#include "lock.h"
#include "net.h"
#include "atm.h"

bool running = true;
int argc;
char **argv;

// should this be a seperate file?
std::vector<std::thread> threads;
lock_t threads_lock;

static int signal_count = 0;

static void init();
static void terminate_();

static void signal_handler(int signal){
	switch(signal){
	case SIGTERM:
		std::cout << "Caught signal SIGTERM, terminating" << std::endl;
		running = false;
		signal_count++;
		break;
	case SIGINT:
		std::cout << "Caught signal SIGINT, terminating" << std::endl;
		running = false;
		signal_count++;
		break;
	default:
		break; // make this more complex
	}
	if(signal_count == 3){
		/*
		  Guaranteed to always kill the program. I need to call
		  whatever TX commands are needed to preserve the 
		  transactions, but I don't know how well the locking
		 */
		throw std::runtime_error("aborting program");
	}
}

static void init(){
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	if(search_for_argv("--help") != -1){
		std::cout << "rpi_btc_atm: raspberry pi based bitcoin atm" << std::endl
			  << "refer to settings.cfg for settings" << std::endl
			  << "usage:" << std::endl
			  << "--debug\t\tdebug output" << std::endl
			  << "--spam\t\tspam output" << std::endl
			  << "--help\t\tdisplay this help screen" << std::endl;
		exit(0);
	}
	settings::set_settings();
	// periodically rgrep with '::init(){" just to make sure I covered all
	// the bases. I don't want to make them variables at this point in time
	tx::init();
	gpio::init();
	qr::init();
	deposit::init();
	atm::init();
}

static void terminate_(){
	print("terminate has been called", P_NOTICE);
	if(running){
		print("Exited main loop while running, terminating safely from here", P_ERR);
		running = false; // stop threads safely
	}
	/*
	  All threads should terminate when running is false
	  Sleep will be interrupted (sleep_ms) if running is false to speed
	  up the process.
	 */
	LOCK_RUN(threads_lock, [](){
			for(unsigned int i = 0;i < threads.size();i++){
				try{
					print("joining thread " + std::to_string(i),
					      P_NOTICE);
					threads[i].join();
				}catch(std::system_error e){
					print("system error for threads",
					      P_ERR);
					if(e.code() == std::errc::invalid_argument){
						print("invalid argument for threads",
						      P_ERR);
					}else if(e.code() == std::errc::no_such_process){
						print("no such process for threads",
						      P_ERR);
					}else if(e.code() == std::errc::resource_deadlock_would_occur){
						print("resource deadlock would occur for threads",
						      P_ERR);
					}else{
						print("unknown error condition for threads",
						      P_ERR);
					}
					throw e;
				}catch(std::exception e){
					pre_pro::exception(e, "thread_join",
							   P_ERR);
					throw e;
				}
			}
			threads.clear();
		}());
	tx::close();
	gpio::close();
	qr::close();
	deposit::close();
	atm::close(); // by extension, drivers
}

static void test_code(){
	/* test things here */
}

static int loop(){
	atm::loop();
	return 0;
}

int main(int argc_, char **argv_){
	argc = argc_;
	argv = argv_;
	init();
	print("init finished", P_NOTICE);
	if(search_for_argv("--test-code") != -1){ // debugging/testing can stay argv
		test_code();
		running = false;
	}
	if(running){
		print("entering main thread", P_NOTICE);
	}
	while(running){
		try{
			loop();
		}catch(const std::exception &e){
			std::cerr << e.what() << std::endl;
			running = false;
		}
	}
	terminate_();
	sleep_ms(1000, true);
	return 0;
}
