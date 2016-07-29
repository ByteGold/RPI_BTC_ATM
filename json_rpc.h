#ifndef JSON_RPC_H
#define JSON_RPC_H
#include "cstdio"
#include "curl/curl.h"
#define DEFAULT_NODE_PORT 8332
#define DEFAULT_NODE_IP "127.0.0.1"
#define JSON_PARENTHESIS(data) ((std::string)("\""+data+"\""))
#define JSON_BRACES(data) ((std::string)("{"+data+"}"))
#define JSON_BRACKETS(data) ((std::string)("["+data+"]"))

#define JSON_TX_ERR_LOW_FEE -4
struct json_rpc_resp_t{
	std::string result;
	std::string error;
	int id;
};
namespace json_rpc{
	int cmd(std::string method, std::vector<std::string> params, int id, std::string ip = DEFAULT_NODE_IP, int port = DEFAULT_NODE_PORT);
	int resp(std::string *result, std::string *error, int id);
	int throw_on_error(int id);
}
extern std::string json_set_var(std::string type, std::vector<std::string> data);
#endif
