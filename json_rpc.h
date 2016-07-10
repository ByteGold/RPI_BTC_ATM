#ifndef JSON_RPC_H
#define JSON_RPC_H
#include "cstdio"
#include "SDL2/SDL_net.h"
#include "curl/curl.h"
namespace json_rpc{
  int send_cmd(std::string method, std::vector<std::string> params, int id, std::string ip, int port);
  int recv_resp(std::string *result, std::string *error, int *id);
}
#endif
