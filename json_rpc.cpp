#include "main.h"
#include "util.h"
#include "json_rpc.h"

#define DEFAULT_NODE_PORT 8332
static int default_node_port = DEFAULT_NODE_PORT;

/*
This interfaces with a Bitcoin node installed on the Raspberry Pi. The Bitcoin
node doesn't have to download the entire blockchain (pruned should work fine).
 */


static std::string json_wrap_with_parenthesis(std::string data){
  return "\""+data+"\"";
}

static std::string json_wrap_with_braces(std::string data){
  return "{"+data+"}";
}

static std::string json_wrap_with_brackets(std::string data){
  return "["+data+"]";
}

#define JSON_PARENTHESIS(data) json_wrap_with_parenthesis(data)
#define JSON_BRACES(data) json_wrap_with_braces(data)
#define JSON_BRACKETS(data) json_wrap_with_brackets(data)

static std::string json_set_var(std::string type, std::string data){
  return JSON_PARENTHESIS(type) + ":" + JSON_PARENTHESIS(data);
}

static std::string json_set_var(std::string type, int data){
  return JSON_PARENTHESIS(type) + ":" + std::to_string(data);
}

static std::string json_set_var(std::string type, std::vector<std::string> data){
  std::string vector_str;
  for(unsigned int i = 0;i < data.size();i++){
    vector_str += JSON_PARENTHESIS(data[i]);
    if(i+1 < data.size()){
      vector_str += ",";
    }
  }
  return JSON_PARENTHESIS(type) + ":" + JSON_BRACKETS(vector_str);
}

static int json_rpc_curl_writeback(char *ptr, size_t size, size_t nmemb, void *userdata){
  print("json_rpc_curl_writeback received " + (std::string)ptr, P_DEBUG);
}

int json_rpc::send_cmd(std::string method, std::vector<std::string> params, int id, std::string ip, int port){
  std::string data = json_set_var("method", method) + ",";
  if(params.size() != 0){
    data += json_set_var("params", params) + ",";
  }
  data += json_set_var("id", id);
  data = json_wrap_with_braces(data);
  print("JSON_RPC query:"+data, P_DEBUG);
  CURL *curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, "http://username:password1@127.0.0.1");
  curl_easy_setopt(curl, CURLOPT_PORT, 8332);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json-rpc");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  CURLcode res = curl_easy_perform(curl);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, json_rpc_curl_writeback);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  headers = NULL;
  curl = NULL;
  return 0;
}

int json_rpc::recv_resp(std::string *result, std::string *error, int *id){
  
  return 0;
}
