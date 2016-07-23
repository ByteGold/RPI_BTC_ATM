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

bool running = true;
int argc;
char **argv;

static std::vector<driver_t*> drivers;
static std::thread driver_run_thread;

static long double current_btc_rate = 0;
static std::thread current_btc_rate_thread;

static void init();
static void terminate();

static void signal_handler(int signal){
  switch(signal){
  case SIGTERM:
    print("Caught signal SIGTERM, terminating", P_CRIT);
    break;
  case SIGKILL:
    print("Caught signal SIGKILL, terminating", P_CRIT);
    break;
  case SIGINT:
    print("Caught signal SIGINT, terminating", P_CRIT);
    break;
  default:
    break; // make this more complex
  }
  running = false;
  terminate();
  exit(0);
}

static void init(){
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGKILL, signal_handler);
  if(search_for_argv("--help") != -1){
    std::cout << "rpi_btc_atm: raspberry pi based bitcoin atm" << std::endl
	      << "refer to settings.cfg for settings" << std::endl
	      << "usage:" << std::endl
	      << "--help\t\tdisplay this help screen" << std::endl;
    exit(0);
  }
  settings::set_settings();
  tx::init();
  gpio::init();
  qr::init();
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
  driver_run_thread = std::thread([](){
    while(running){
      for(unsigned int i = 0;i < drivers.size();i++){
	LOCK_RUN(drivers[i]->lock, drivers[i]->run(&drivers[i]->count));
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5)); // reasonable
    }
    });
  current_btc_rate_thread = std::thread([](){
    while(running){
      try{
	current_btc_rate = get_btc_rate(settings::get_setting("currency"));
      }catch(...){
	print("error in updating the price, probably downtime", P_ERR);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5000));
      // I don't want to get the ATM banned, so don't speed it up
    }
    });
}

static void terminate(){
  if(running){
    print("Exited main loop while running, terminating safely from here", P_ERR);
    running = false; // stop threads safely
  }
  for(unsigned int i = 0;i < drivers.size();i++){
    drivers[i]->close();
    delete drivers[i];
  }
  drivers.clear();
  driver_run_thread.join();
  tx::close();
  gpio::close();
  qr::close();
}

static void test_code(){
  json_rpc::cmd("getdifficulty", {}, 1, "127.0.0.1", 8332);
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
    // print current amount of money to screen, exchange rate, markup, etc.
    while(accepting_money){
      for(unsigned int i = 0;i < drivers.size();i++){
	if(drivers[i]->count != old_count){
	  old_count += drivers[i]->count;
	  LOCK_RUN(drivers[i]->lock, drivers[i]->count = 0);
	  time_since_last_change = std::time(nullptr); // epoch
	}
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
    return 0;
}

int main(int argc_, char **argv_){
  argc = argc_;
  argv = argv_;
  init();
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
      // don't use print due to possible locking problems
      running = false;
    }
  }
  terminate();
  return 0;
}
