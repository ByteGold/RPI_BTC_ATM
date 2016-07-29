#include "main.h"
#include "util.h"
#include "json_rpc.h"
#include "settings.h"

static int json_rpc_port = DEFAULT_NODE_PORT;
static std::string json_rpc_username;
static std::string json_rpc_password;

static std::vector<json_rpc_resp_t> responses;
static std::mutex responses_lock;

/*
  This interfaces with a Bitcoin node installed on the Raspberry Pi. The Bitcoin
  node doesn't have to download the entire blockchain (pruned should work fine).
  Having said that, a 4GB card should be more than enough (I use 2GB)
*/


static std::string json_set_var(std::string type, std::string data){
	return JSON_PARENTHESIS(type) + ":" + JSON_PARENTHESIS(data);
}

static std::string json_set_var(std::string type, int data){
	return JSON_PARENTHESIS(type) + ":" + std::to_string(data);
}

static bool json_is_number(std::string a){
	for(unsigned int i = 0;i < a.size();i++){
		switch(a[i]){
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
			break;
		default:
			return false;
		}
	}
	return true;
}

std::string json_set_var(std::string type, std::vector<std::string> data){
	std::string vector_str;
	for(unsigned int i = 0;i < data.size();i++){
		if(json_is_number(data[i])){
			print("treating number ("+data[i]+") as a non-string, not quoting", P_DEBUG);
			vector_str += data[i]; // data is a number
		}else{
			vector_str += JSON_PARENTHESIS(data[i]);
		}
		if(i+1 < data.size()){
			vector_str += ",";
		}
	}
	return JSON_PARENTHESIS(type) + ":" + JSON_BRACKETS(vector_str);
}

/*
  simple function to find the value to a variable in JSON
  Does NOT recognize braces or brackets, just searches
 */

static std::string json_simple_find_var(const std::string json_raw, const std::string var){
	std::string retval;
	const size_t var_start = json_raw.find(var);
	if(var_start == std::string::npos){
		print("unable to find var in json_raw", P_ERR);
		throw std::runtime_error("no var in json_raw");
	}
	const size_t start_pos = var_start + var.size();
	size_t cur_pos = start_pos, end_pos = 0;
	char ending_char = 0;
	for(;cur_pos < json_raw.size();cur_pos++){
		if(json_raw[cur_pos] == ':'){
			cur_pos++;
			ending_char = json_raw[cur_pos];
			break;
		}
	}
	end_pos = cur_pos; // skip past the colon
	if(ending_char != '\"'){
		ending_char = ',';
		// could be wrong for braces, but that is taken care of
		// in the code below
	}
	bool end_of_var = false;
	while(end_of_var == false){
		const char current_char = json_raw[end_pos];
		if(end_pos != start_pos){
			const bool end_of_last = current_char == '}';
			const bool end_of_json = current_char == ending_char;
			if(end_of_json || end_of_last){
				end_of_var = true;
				continue;
			}
		}
		end_pos++;
	}
	if(ending_char == '\"'){
		end_pos++;
		// ending_char is in the front, so uniformity means it stays
	}
	try{
		retval = json_raw.substr(cur_pos, end_pos-cur_pos);
	}catch(std::out_of_range e){
		print("out of range for json_simple_find_var retval", P_ERR);
		throw e;
	}catch(std::bad_alloc e){
		print("bad alloc for json_simple_find_var retval", P_ERR);
		throw e;
	}catch(std::exception e){
		print("unknown exception for json_simple_find_var retval", P_ERR);
		throw e;
	}
	P_V_S(var, P_DEBUG);
	P_V(start_pos, P_DEBUG);
	P_V(end_pos, P_DEBUG);
	P_V(cur_pos, P_DEBUG);
	P_V_C(ending_char, P_DEBUG);
	P_V_S(retval, P_DEBUG);
	return retval;
}

static int json_rpc_curl_writeback(char *ptr, size_t size, size_t nmemb, void *userdata){
	if(userdata != nullptr){
		print("userdata is not a nullptr, why?", P_ERR);
	}
	if(size != 1){
		print("multiple requests at one time", P_ERR);
		// shouldn't happen, we aren't near breaking any MTU
	}	
	print("json_rpc_curl_writeback received " + (std::string)ptr, P_DEBUG);
	json_rpc_resp_t response;
	std::string raw_json = std::string(ptr, nmemb);
	print("interpreted raw_json as '" + raw_json + "'", P_DEBUG);
	try{
		response.result = json_simple_find_var(raw_json, "result");
		print("interpreted response.result as '" + response.result + "'", P_DEBUG);
	}catch(std::out_of_range e){
		print("out of range for response.result", P_ERR);
	}catch(std::bad_alloc e){
		print("bad alloc for response.result", P_ERR);
	}catch(...){
		print("unknown exception for response.result", P_ERR);
	}
	try{
		
		response.error = json_simple_find_var(raw_json, "code");
		print("interpreted response.error as '" + response.error + "'", P_DEBUG);
	}catch(std::out_of_range e){
		print("out of range for response.error", P_ERR);
	}catch(std::bad_alloc e){
		print("bad alloc for response.error", P_ERR);
	}catch(...){
		print("unknown exception for response.error", P_ERR);
	}
	try{
		response.id = std::stoi(json_simple_find_var(raw_json, "id"));
	}catch(std::invalid_argument e){
		print("invalid argument for response.id", P_ERR);
	}catch(std::out_of_range e){
		print("out of range for response.id", P_ERR);
	}catch(...){
		print("unknown exception for response.id", P_ERR);
	}
	LOCK_RUN(responses_lock, [](json_rpc_resp_t response){
			for(unsigned int i = 0;i < responses.size();i++){
				if(responses[i].id == response.id){
					responses.erase(responses.begin()+i);
					// shouldn't be a difference between deleting vs setting right now
				}
			}
			responses.push_back(response);
		}(response));
	return nmemb*size; // return bytes taken care of
}

static int json_rpc_send_query(std::string url, std::string json_query){
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_query.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_query.size());
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json-rpc");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_perform(curl);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, json_rpc_curl_writeback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);
	if(search_for_argv("--debug") != -1 || search_for_argv("--spam") != -1){
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	}else{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	}
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);
	headers = NULL;
	curl = NULL;
	return 0;
}

int json_rpc::cmd(std::string method, std::vector<std::string> params, int id, std::string ip, int port){
	if(json_rpc_username == ""){
		if(settings::get_setting("json_rpc_username") != ""){
			json_rpc_username = settings::get_setting("json_rpc_username");
		}
	}
	if(json_rpc_password == ""){
		if(settings::get_setting("json_rpc_password") != ""){
			json_rpc_password = settings::get_setting("json_rpc_password");
		}
	}
	print("json_rpc_username:" + json_rpc_username, P_DEBUG);
	print("json_rpc_password:" + json_rpc_password, P_DEBUG);
	if(json_rpc_port == 0){
		if(settings::get_setting("json_rpc_port") != ""){
			try{
				json_rpc_port = std::stoi(settings::get_setting("json_rpc_port"));
			}catch(std::invalid_argument e){
				print("invalid argument for json_rpc_port, check settings.cfg file", P_ERR);
				json_rpc_port = DEFAULT_NODE_PORT; // 8332
			}catch(std::out_of_range e){
				print("out of range for json_rpc_port, check settings.cfg file", P_ERR);
				json_rpc_port = DEFAULT_NODE_PORT;
			}catch(...){
				print("unknown exception for json_rpc_port, check settings.cfg file", P_ERR);
				json_rpc_port = DEFAULT_NODE_PORT;
			}
		}
	}
	std::string data;
	data += json_set_var("method", method) + ",";
	if(params.size() != 0){
		data += json_set_var("params", params) + ",";
	}
	data += json_set_var("id", id);
	data = JSON_BRACES(data);
	print("json_rpc query:"+data, P_DEBUG);
	json_rpc_send_query("http://"+json_rpc_username+":"+json_rpc_password+"@"+ip+":"+std::to_string(port), data);
	//to_string is pretty safe as it is
	return 0;
}

int json_rpc::resp(std::string *result, std::string *error, int id){
	if(result == nullptr){
		print("result is nullptr for resp", P_ERR);
		return -1;
	}
	if(error == nullptr){
		print("error is nullptr for resp", P_ERR);
		return -1;
	}
	LOCK_RUN(responses_lock, [](std::string *result, std::string *error, int id){
			for(unsigned int i = 0;i < responses.size();i++){
				if(responses[i].id == id){
					*result = responses[i].result;
					*error = responses[i].error;
					responses.erase(responses.begin()+i);
				}
			}
		}(result, error, id));
	if(*result != "" && *error != ""){
		print("json_rpc result for id=" + std::to_string(id) + ":" + *result + " " + *error, P_DEBUG);
		return 0;
	}
	return -1;
}

int json_rpc::throw_on_error(int id){
	std::string result, code_str;
	resp(&result, &code_str, id);
	int code = 0; // 0 means no error?
	try{
		code = std::stoi(code_str);
	}catch(std::invalid_argument e){
		print("invalid argument for throw_on_error", P_ERR);
	}catch(std::out_of_range e){
		print("out of range for throw_on_error", P_ERR);
	}catch(...){
		print("unknown exception for throw_on_error", P_ERR);
	}
	print("json_rpc error code is " + std::to_string(code), P_ERR);
	throw code;
}
