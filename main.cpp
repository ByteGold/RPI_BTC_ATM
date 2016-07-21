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

static std::string currency = "USD";
static std::vector<driver_t*> drivers;
static std::thread driver_run_thread;

static void init();
static void terminate();

static void driver_run(){
  while(running){
    for(unsigned int i = 0;i < drivers.size();i++){
      LOCK_RUN(drivers[i]->lock, drivers[i]->run(&drivers[i]->count));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5)); // reasonable
  }
}

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
  gpio::init();
  qr::init();
  currency = settings::get_setting("currency");
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
  driver_run_thread = std::thread(driver_run);
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
  gpio::close();
  qr::close();
}

static void test_code(){
  json_rpc::cmd("getdifficulty", {}, 1, "127.0.0.1", 8332);
}

int main(int argc_, char **argv_){
  argc = argc_;
  argv = argv_;
  init();
  if(search_for_argv("--test-code") != -1){ // debugging/testing can stay argv
    test_code();
    running = false;
  }
  print("entering main thread", P_NOTICE);
  while(running){
    for(unsigned int i = 0;i < drivers.size();i++){
      if(drivers[i]->count != 0){
  	std::cout << "$" << (float)(((float)drivers[i]->count)/100.0) << std::endl;
  	LOCK_RUN(drivers[i]->lock,drivers[i]->count = 0;);
      }
    }
  }
  terminate();
  return 0;
}
