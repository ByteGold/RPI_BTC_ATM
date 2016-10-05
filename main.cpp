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

bool running = true;
int argc;
char **argv;

std::vector<std::thread> threads;
lock_t threads_lock;

static std::vector<driver_t*> drivers;
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
	// TODO: convert from parameters to cfg file
	if(settings::get_setting("ch_926_drv") == "true"){
		print("Enabling CH 926 driver", P_NOTICE);
		driver_t *ch_926 = new driver_t;
		ch_926->init = ch_926_init;
		ch_926->close = ch_926_close;
		ch_926->run = ch_926_run;
		ch_926->reject_all = ch_926_reject_all;
		ch_926->accept_all = ch_926_accept_all;
		drivers.push_back(ch_926);
	}
	for(unsigned int i = 0;i < drivers.size();i++){
		drivers[i]->init();
	}
	// doesn't need the lock, haven't built a function that handles
	// functions with commas or other special characters
	threads.emplace_back(std::thread([](){
		while(running){
			for(unsigned int i = 0;i < drivers.size();i++){
				LOCK_RUN(drivers[i]->lock, drivers[i]->run(&drivers[i]->count));
			}
			sleep_ms(DEFAULT_THREAD_SLEEP);
		}
	}));
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
					print("joining thread " + std::to_string(i), P_NOTICE);
					threads[i].join();
				}catch(std::system_error e){
					print("system error for threads", P_ERR);
					if(e.code() == std::errc::invalid_argument){
						print("invalid argument for threads", P_ERR);
					}else if(e.code() == std::errc::no_such_process){
						print("no such process for threads", P_ERR);
					}else if(e.code() == std::errc::resource_deadlock_would_occur){
						print("resource deadlock would occur for threads", P_ERR);
					}else{
						print("unknown error condition for threads", P_ERR);
					}
					throw e;
				}catch(std::exception e){
					pre_pro::exception(e, "thread_join", P_ERR);
					throw e;
				}
			}
			threads.clear();
		}());
	for(unsigned int i = 0;i < drivers.size();i++){
		drivers[i]->close();
		delete drivers[i];
	}
	drivers.clear();
	tx::close();
	gpio::close();
	qr::close();
	deposit::close();
}

static void test_code(){
	while(running){
		std::cout << "OUTPUT:" << system_cmd_output("echo hello")
			  << std::endl;
	}
}

static int loop(){
	std::string btc_addr = qr::read_from_webcam();
	// print to screen "SCAN WALLET"
	if(btc_addr == ""){
		return 0;
	}
	if(btc_addr.substr(0, 8) == "bitcoin:"){
		btc_addr = btc_addr.substr(8, btc_addr.size());
		print("removed 'bitcoin:' prefix from address (" + btc_addr +")", P_NOTICE);
	}
	int old_count = 0;
	long int time_since_last_change = std::time(nullptr);
	bool accepting_money = true;
	while(accepting_money){
		for(unsigned int i = 0;i < drivers.size();i++){
			LOCK_RUN(drivers[i]->lock,
				 if(drivers[i]->count != old_count){
					 old_count += drivers[i]->count;
					 drivers[i]->count = 0;
					 time_since_last_change = std::time(nullptr); // epoch
				 });
		}
		if(std::time(nullptr)-time_since_last_change >= 5){ // rough estimate
			print("money deposit timeout reached, depositing the money", P_NOTICE);
			// print "TIMEOUT REACHED" \n "SENDING BTC" to screen
			accepting_money = false;
		}
	}
	if(old_count != 0){
		if(deposit::send(btc_addr, old_count*get_btc_rate(settings::get_setting("currency")) == 0)){
			// print "BTC SENT" to screen
			print("tx seemed to go properly", P_NOTICE);
		}else{
			// print "ERROR IN TX" \n "RETRYING LATER"
			print("error in tx", P_ERR);
		}
	}
	sleep_ms(1000);
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
