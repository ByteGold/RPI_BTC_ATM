#include "main.h"
#include "util.h"
#include "json_rpc.h"

static int json_rpc_port = DEFAULT_NODE_PORT;

static std::string json_rpc_username;
static std::string json_rpc_password;

static std::vector<json_rpc_resp_t> responses;

/*
  This interfaces with a Bitcoin node installed on the Raspberry Pi. The Bitcoin
  node doesn't have to download the entire blockchain (pruned should work fine).
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

static int json_rpc_curl_writeback(char *ptr, size_t size, size_t nmemb, void *userdata){
  print("json_rpc_curl_writeback received " + (std::string)ptr, P_DEBUG);
  //TODO: parse output and put it into responses vector
  return 0;
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
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  headers = NULL;
  curl = NULL;
  return 0;
}

int json_rpc::cmd(std::string method, std::vector<std::string> params, int id, std::string ip, int port){
  if(search_for_argv("--json-rpc-username") != -1){
    json_rpc_username = get_argv(search_for_argv("--json-rpc-username")+1);
  }
  if(search_for_argv("--json-rpc-password") != -1){
    json_rpc_password = get_argv(search_for_argv("--json-rpc-password")+1);
  }
  if(search_for_argv("--json-rpc-port") != -1){
    json_rpc_port = std::stoi(get_argv(search_for_argv("--json-rpc-port")+1));
  }
  std::string data;
  data += json_set_var("method", method) + ",";
  if(params.size() != 0){
    data += json_set_var("params", params) + ",";
  }
  data += json_set_var("id", id);
  data = JSON_BRACES(data);
  print("JSON_RPC query:"+data, P_DEBUG);
  json_rpc_send_query("http://"+json_rpc_username+":"+json_rpc_password+"@"+ip+":"+std::to_string(port), data);
  return 0;
}

int json_rpc::resp(std::string *result, std::string *error, int id){
  for(unsigned int i = 0;i < responses.size();i++){
    if(responses[i].id == id){
      *result = responses[i].result;
      *error = responses[i].error;
      responses.erase(responses.begin()+i);
      return 0;
    }
  }
  return -1;
}
