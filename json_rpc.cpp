#include "main.h"
#include "util.h"
#include "json_rpc.h"
#include "settings.h"
#include "lock.h"

static int json_rpc_port = DEFAULT_NODE_PORT;
static std::string json_rpc_username;
static std::string json_rpc_password;

static std::vector<json_rpc_query_t> queries;
static lock_t queries_lock;

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
	auto queries_function = [](char *ptr, size_t size, size_t nmemb){
		std::string raw_json = std::string(ptr, size*nmemb);
		int tmp_id = -1;
		long int query_entry = -1;
		print("interpreted raw_json as '" + raw_json + "'", P_DEBUG);
		try{
			tmp_id = std::stoi(json_simple_find_var(raw_json, "id"));
			print("interpreted tmp_id as " + std::to_string(tmp_id), P_DEBUG);
		}catch(std::invalid_argument e){
			print("invalid argument for tmp_id", P_ERR);
			return;
		}catch(std::out_of_range e){
			print("out of range for tmp_id", P_ERR);
			return;
		}catch(...){
			print("unknown exception for tmp_id", P_ERR);
			return;
		}
		for(unsigned int i = 0;i < queries.size();i++){
			if(queries[i].id == tmp_id){
				query_entry = (long int)i;
				break;
			}else{
				P_V(queries[i].id, P_SPAM);
				P_V(tmp_id, P_SPAM);
			}
		}
		if(query_entry == -1){
			print("cannot find json query in queries list", P_ERR);
			throw std::runtime_error("no json query in vector tmp_id:" + std::to_string(tmp_id));
		}else{
			print("found json query", P_DEBUG);
		}
		try{
			queries[query_entry].result = json_simple_find_var(raw_json, "result");
			print("interpreted queries[query_entry].result as '" + queries[query_entry].result + "'", P_DEBUG);
		}catch(std::out_of_range e){
			print("out of range for queries[query_entry].result", P_ERR);
		}catch(std::bad_alloc e){
			print("bad alloc for queries[query_entry].result", P_ERR);
		}catch(...){
			print("unknown exception for queries[query_entry].result", P_ERR);
		}
		try{
			queries[query_entry].error_code = std::stoi(json_simple_find_var(raw_json, "code"));
			print("interpreted queries[query_entry].error_code as '" + std::to_string(queries[query_entry].error_code) + "'", P_DEBUG);
		}catch(std::invalid_argument e){
			print("invalid argument for queries[query_entry].error_code", P_ERR);
		}catch(std::out_of_range e){
			print("out of range for queries[query_entry].error_code", P_ERR);
		}catch(...){
			print("unknown exception for queries[query_entry].error_code", P_ERR);
		}
		int query_status = json_rpc::error_check(queries[query_entry].id);
		if(query_status == 0){ // check if this is no error
			queries[query_entry].status = JSON_RPC_QUERY_OK;
		}else{
			queries[query_entry].status = JSON_RPC_QUERY_ERROR;
		}
	};
	auto queries_function_pass = std::bind(queries_function, ptr, size, nmemb);
	LOCK_RUN(queries_lock, queries_function_pass());
	return nmemb*size; // return bytes taken care of
}

/*
  Designed to only send one at a time, reads from oldest to newest
 */

static int json_rpc_send_query_block(std::string url){
	std::string query_data;
	CURL *curl = curl_easy_init();
	std::vector<std::string> queries_list;
	LOCK_RUN(queries_lock,
		print("queries.size():" + std::to_string(queries.size()), P_SPAM);
		for(unsigned int i = 0;i < queries.size();i++){
			queries_list.push_back(queries[i].query);
			queries[i].status = JSON_RPC_QUERY_SENT;
		});
	for(unsigned int i = 0;i < queries_list.size();i++){
		const std::string json_query = queries_list[i];
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_query.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_query.size());
		struct curl_slist *headers = NULL;
		curl_slist_append(headers, "Content-Type: application/json-rpc");
		if(search_for_argv("--debug") != -1 || search_for_argv("--spam") != -1){
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		}else{
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
		}
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_perform(curl);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, json_rpc_curl_writeback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);
		curl_easy_perform(curl); // json_rpc_curl_writeback has its own lock
		curl_slist_free_all(headers);
		headers = NULL;
	}
	curl_easy_cleanup(curl);
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
	queries.push_back(json_rpc_query_t(data, id));
	const int queries_entry = queries.size()-1;
	json_rpc_send_query_block("http://"+json_rpc_username+":"+json_rpc_password+"@"+ip+":"+std::to_string(port));
	bool received_response = false;
	long int starting_time = std::time(nullptr);
	/*
	  Safe and reliable to look up query on main thread, because the function
	  doesn't return until a response is found or a timeout is reached
	 */
	while(received_response == false){
		if(std::time(nullptr)-starting_time == 5){
			print("json_rpc timeout exceeded, exiting w/o response", P_WARN);
			break;
		}
		LOCK_RUN(queries_lock, if(queries[queries_entry].status != JSON_RPC_QUERY_SENT){
				print("received response for json_rpc query", P_NOTICE);
				received_response = true;
			});
	}
	//to_string is pretty safe as it is
	return 0;
}

// -1 on not available yet (completely)
// 0 on success
int json_rpc::resp(std::string *result, int *error_code, int id){
	if(result == nullptr){
		print("result is nullptr for resp", P_ERR);
		return -1;
	}
	if(error_code == nullptr){
		print("error is nullptr for resp", P_ERR);
		return -1;
	}
	*error_code = 0;
	LOCK_RUN(queries_lock, [](std::string *result, int *error_code, int id){
			for(unsigned int i = 0;i < queries.size();i++){
				if(queries[i].id == id){
					*result = queries[i].result;
					*error_code = queries[i].error_code;
				}
			}
		}(result, error_code, id));
	if(*result != ""){
		print("json_rpc result for id=" + std::to_string(id) + ":" + *result + " " + std::to_string(*error_code), P_DEBUG);
		return 0;
	}
	return -1;
}

int json_rpc::throw_on_error(int id){
	std::string result;
	int error_code;
	resp(&result, &error_code, id);
	print("json_rpc error code is " + std::to_string(error_code), P_ERR);
	if(error_code != 0){
		throw error_code;
	}
	return error_code;
}

int json_rpc::error_check(int id){
	int retval = 0;
	try{
		throw_on_error(id);
	}catch(const int e){
		retval = e;
		switch(e){
		case RPC_INVALID_REQUEST:
			print("invalid request for throw_on_error", P_ERR);
			break;
		case RPC_METHOD_NOT_FOUND:
			print("method not found for throw_on_error", P_ERR);
			break;
		case RPC_INVALID_PARAMS:
			print("invalid params for throw_on_error", P_ERR);
			break;
		case RPC_INTERNAL_ERROR:
			print("internal error for throw_on_error", P_ERR);
			break;
		case RPC_PARSE_ERROR:
			print("parse error for throw_on_error", P_ERR);
			break;
		case RPC_MISC_ERROR:
			print("misc error for throw_on_error", P_ERR);
			break;
		case RPC_FORBIDDEN_BY_SAFE_MODE:
			print("forbidden by safe mode for throw_on_error", P_ERR);
			break;
		case RPC_TYPE_ERROR:
			print("type error for throw_on_error", P_ERR);
			break;
		case RPC_INVALID_ADDRESS_OR_KEY:
			print("invalid address or key for throw_on_error", P_ERR);
			break;
		case RPC_OUT_OF_MEMORY:
			print("out of memory for throw_on_error", P_ERR);
			break;
		case RPC_INVALID_PARAMETER:
			print("invalid parameter for throw_on_error", P_ERR);
			break;
		case RPC_DATABASE_ERROR:
			print("database error for throw_on_error", P_ERR);
			break;
		case RPC_DESERIALIZATION_ERROR:
			print("deserialization error for throw_on_error", P_ERR);
			break;
		case RPC_VERIFY_ERROR:
			print("verify error for throw_on_error", P_ERR);
			break;
		case RPC_VERIFY_REJECTED:
			print("verify rejected for throw_on_error", P_ERR);
			break;
		case RPC_VERIFY_ALREADY_IN_CHAIN:
			print("already in chain for throw_on_error", P_ERR);
			break;
		case RPC_IN_WARMUP:
			print("in warmup for throw_on_error", P_ERR);
			break;
		case RPC_CLIENT_NOT_CONNECTED:
			print("client not connected for throw_on_error", P_ERR);
			break;
		case RPC_CLIENT_IN_INITIAL_DOWNLOAD:
			print("client in initial download for throw_on_error", P_ERR);
			break;
		case RPC_CLIENT_NODE_ALREADY_ADDED:
			print("node already added for throw_on_error", P_ERR);
			break;
		case RPC_CLIENT_NODE_NOT_ADDED:
			print("node not added for throw_on_error", P_ERR);
			break;
		case RPC_CLIENT_NODE_NOT_CONNECTED:
			print("node not connected for throw_on_error", P_ERR);
			break;
		case RPC_CLIENT_INVALID_IP_OR_SUBNET:
			print("invalid ip or subnet for throw_on_error", P_ERR);
			break;
		case RPC_WALLET_ERROR:
			print("wallet error for throw_on_error", P_ERR);
			break;
		case RPC_WALLET_INSUFFICIENT_FUNDS:
			print("insufficient funds for throw_on_error", P_ERR);
			break;
		case RPC_WALLET_INVALID_ACCOUNT_NAME:
			print("invalid account name for throw_on_error", P_ERR);
			break;
		case RPC_WALLET_KEYPOOL_RAN_OUT:
			print("keypool ran out for throw_on_error", P_ERR);
			break;
		case RPC_WALLET_UNLOCK_NEEDED:
			print("wallet unlock needed for throw_on_error", P_ERR);
			break;
		case RPC_WALLET_PASSPHRASE_INCORRECT:
			print("passphrase incorrect for throw_on_error", P_ERR);
			break;
		case RPC_WALLET_WRONG_ENC_STATE:
			print("wrong enc state for throw_on_error", P_ERR);
			break;
		case RPC_WALLET_ENCRYPTION_FAILED:
			print("encryption failed for throw_on_error", P_ERR);
			break;
		case RPC_WALLET_ALREADY_UNLOCKED:
			print("wallet already unlocked for throw_on_error", P_ERR);
			break;
		default:
			print("unknown exception for throw_on_error", P_ERR);
			break;
		}
	}catch(...){
		print("unknown exception for throw_on_error", P_ERR);
	}
	return retval;
}

json_rpc_query_t::json_rpc_query_t(std::string data, int id_){
	query = data;
	id = id_;
	P_V_S(query, P_DEBUG);
	P_V(id, P_DEBUG);
}

json_rpc_query_t::~json_rpc_query_t(){
}
