#include "main.h"
#include "util.h"
#include "file.h"
#include "lock.h"
#include "net.h"
#include "settings.h"

int print_level = -1;

std::vector<std::string> newline_to_vector(std::string data){
	std::vector<std::string> retval;
	unsigned long int old_pos = 0;
	for(unsigned int i = 0;i < data.size();i++){
		if(data[i] == '\n'){
			const std::string tmp =
				data.substr(old_pos, i);
			old_pos = i+1; // skip newline
			retval.push_back(tmp);
		}
	}
	return retval;

}

void sleep_ms(int ms, bool force){
	if(force){
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}else{
		for(int i = 0;i < ms;i++){
			/*
			  when the program needs to quit quickly for whatever
			  reason, this stops the sleep and lets the program run.
			  maybe it should throw an exception?
			 */
			const auto sleep_time = std::chrono::milliseconds(1);
			std::this_thread::sleep_for(sleep_time);
			if(running == false){
				return;
			}
		}
	}
}

int search_for_argv(std::string value){
	for(int i = 0;i < argc;i++){
		if(std::strcmp(argv[i], value.c_str()) == 0){
			return i;
		}
	}
	return -1;
}

static std::string print_level_text(int level){
	switch(level){
	case P_SPAM:
		return "[SPAM]";
	case P_DEBUG:
		return "[DEBUG]";
	case P_NOTICE:
		return "[NOTICE]";
	case P_WARN:
		return "[WARN]";
	case P_ERR:
		return "[ERROR]";
	case P_CRIT:
		return "[CRITICAL]";
	}
	throw std::runtime_error("invalid print level");
}

lock_t print_lock;

void print(std::string data, int level, const char *func){
	LOCK_RUN(print_lock, [&](){
			if(print_level == -1){
				if(search_for_argv("--debug") != -1){
					print_level = P_DEBUG;
				}else if(search_for_argv("--spam") != -1){
					print_level = P_SPAM;
				}else{
					print_level = P_NOTICE; // sane minimum
				}
			}
			if(level >= print_level){
				std::string func_;
				if(func != nullptr){
					func_ = func;
				}
				std::cout << print_level_text(level) << " ";
				std::cout << func_ << " ";
				std::cout << data << std::endl;
			}
			if(level == P_CRIT){
				std::cerr << "CRITICAL ERROR" << std::endl;
				throw std::runtime_error(data);
			}
		}());
}

/*
  Should I use a real-time currency conversion API or just use different basic
  APIs like this for each currency? I guess this is good enough until this has
  some major adoption, or at least interest, because adoption can be hindered.
 */

long double get_btc_rate(std::string currency){
	if(currency == "USD" || currency == "usd"){
		int stale_time = 30; // good baseline
		try{
			const std::string stale_str = 
				settings::get_setting("btc_rate_stale_time");
			stale_time = std::stoi(stale_str);
		}catch(std::exception e){
			pre_pro::exception(e, "get_btc_rate_settings", P_ERR);
		}
		const std::string str =
			net::get("https://blockchain.info/q/24hrprice",
				stale_time);
		long double retval = 0;
		try{
			retval = std::stold(str);
		}catch(std::exception e){
			pre_pro::exception(e, "get_btc_rate", P_ERR);
		}
		return retval;
	}else{
		print("your plebian currency isn't supported yet", P_CRIT);
		return 0; // complains about no retval
	}
}

int system_(std::string str){
	print("system: " + str, P_DEBUG);
	/*
	  Most commands need some time to be processed on the lower level (GPIO).
	  Speed shouldn't be a problem
	  Possibly append ';touch finished' and wait for the file?
	*/
	int retval = system(str.c_str());
	sleep_ms(50);
	return retval;
}

int system_write(std::string command, std::string file){
	int retval = system_(command + " > " + file);
	while(file::exists(file) == false){
		sleep_ms(1);
	}
	return retval;
}

int system_wait_for_file(std::string command, std::string file){
	int retval = system_(command);
	while(file::exists(file) == false){
		sleep_ms(1);
	}
	return retval;
}

/*
  I heard some bad things about the C-way of getting
  command output, so this seems like the best way to pipe
  it into a variable
 */

std::string system_cmd_output(std::string cmd){
	system_write(cmd, "TMP_OUT");
	const std::string file_data = file::read_file("TMP_OUT");
	system_("rm TMP_OUT");
	return file_data;
}

static std::string to_lower(std::string a){
	std::string retval = a;
	for(int i = 0;i < (int)retval.size();i++){
		retval[i] = std::tolower(retval[i]);
	}
	return retval;
}

long double get_mul_to_btc(std::string currency){
	long double mul = -1;
	if(to_lower(currency) == "satoshi"){
		mul = 1.0/100000000;
	}else if(to_lower(currency) == "cbtc"){
		mul = 1.0/100;
	}else if(to_lower(currency) == "mbtc"){
		mul = 1.0/1000;
	}else if(to_lower(currency) == "ubtc" || to_lower(currency) == "bit"){
		mul = 1.0/1000000;
	}else if(to_lower(currency) == "btc"){
		mul = 1.0;
	}else{
		throw std::runtime_error("invalid currency");
	}
	if(mul == 0){
		throw std::logic_error("mul == 0");
	}
	if(mul < 0){
		throw std::logic_error("mul < 0");
	}
	return mul;
}

void pre_pro::unable(std::string from, std::string to, int level){
	print("unable to get " + to + " from " + from,  level);
}

void pre_pro::exception(std::exception e, std::string for_, int level){
	print((std::string)e.what() + " for " + for_, level); 
}
