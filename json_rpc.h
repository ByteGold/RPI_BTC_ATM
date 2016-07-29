#ifndef JSON_RPC_H
#define JSON_RPC_H
#include "cstdio"
#include "curl/curl.h"
#define DEFAULT_NODE_PORT 8332
#define DEFAULT_NODE_IP "127.0.0.1"
#define JSON_PARENTHESIS(data) ((std::string)("\""+data+"\""))
#define JSON_BRACES(data) ((std::string)("{"+data+"}"))
#define JSON_BRACKETS(data) ((std::string)("["+data+"]"))

/*
  Copied from local (1.1.0) version of Bitcoin Classic
 */

//! Standard JSON-RPC 2.0 errors
#define RPC_INVALID_REQUEST   -32600
#define RPC_METHOD_NOT_FOUND  -32601
#define RPC_INVALID_PARAMS    -32602
#define RPC_INTERNAL_ERROR    -32603
#define RPC_PARSE_ERROR       -32700

//! General application defined errors
#define RPC_MISC_ERROR                   -1  //! std::exception thrown in command handling
#define RPC_FORBIDDEN_BY_SAFE_MODE       -2  //! Server is in safe mode, and command is not allowed in safe mode
#define RPC_TYPE_ERROR                   -3  //! Unexpected type was passed as parameter
#define RPC_INVALID_ADDRESS_OR_KEY       -5  //! Invalid address or key
#define RPC_OUT_OF_MEMORY                -7  //! Ran out of memory during operation
#define RPC_INVALID_PARAMETER            -8  //! Invalid, missing or duplicate parameter
#define RPC_DATABASE_ERROR               -20 //! Database error
#define RPC_DESERIALIZATION_ERROR        -22 //! Error parsing or validating structure in raw format
#define RPC_VERIFY_ERROR                 -25 //! General error during transaction or block submission
#define RPC_VERIFY_REJECTED              -26 //! Transaction or block was rejected by network rules
#define RPC_VERIFY_ALREADY_IN_CHAIN      -27 //! Transaction already in chain
#define RPC_IN_WARMUP                    -28 //! Client still warming up

//! Aliases for backward compatibility
#define RPC_TRANSACTION_ERROR            RPC_VERIFY_ERROR
#define RPC_TRANSACTION_REJECTED         RPC_VERIFY_REJECTED
#define RPC_TRANSACTION_ALREADY_IN_CHAIN RPC_VERIFY_ALREADY_IN_CHAIN

//! P2P client errors
#define RPC_CLIENT_NOT_CONNECTED         -9  //! Bitcoin is not connected
#define RPC_CLIENT_IN_INITIAL_DOWNLOAD   -10 //! Still downloading initial blocks
#define RPC_CLIENT_NODE_ALREADY_ADDED    -23 //! Node is already added
#define RPC_CLIENT_NODE_NOT_ADDED        -24 //! Node has not been added before
#define RPC_CLIENT_NODE_NOT_CONNECTED    -29 //! Node to disconnect not found in connected nodes
#define RPC_CLIENT_INVALID_IP_OR_SUBNET  -30 //! Invalid IP/Subnet

//! Wallet errors
#define RPC_WALLET_ERROR                 -4  //! Unspecified problem with wallet (key not found etc.)
#define RPC_WALLET_INSUFFICIENT_FUNDS    -6  //! Not enough funds in wallet or account
#define RPC_WALLET_INVALID_ACCOUNT_NAME  -11 //! Invalid account name
#define RPC_WALLET_KEYPOOL_RAN_OUT       -12 //! Keypool ran out, call keypoolrefill first
#define RPC_WALLET_UNLOCK_NEEDED         -13 //! Enter the wallet passphrase with walletpassphrase first
#define RPC_WALLET_PASSPHRASE_INCORRECT  -14 //! The wallet passphrase entered was incorrect
#define RPC_WALLET_WRONG_ENC_STATE       -15 //! Command given in wrong wallet encryption state (encrypting an encrypted wallet etc.)
#define RPC_WALLET_ENCRYPTION_FAILED     -16 //! Failed to encrypt the wallet
#define RPC_WALLET_ALREADY_UNLOCKED      -17 //! Wallet is already unlocked

/*
  End copy
 */

struct json_rpc_resp_t{
	std::string result;
	std::string error;
	int id;
};
namespace json_rpc{
	int cmd(std::string method, std::vector<std::string> params, int id, std::string ip = DEFAULT_NODE_IP, int port = DEFAULT_NODE_PORT);
	int resp(std::string *result, std::string *error, int id);
	int throw_on_error(int id);
	int error_check(int id);
}
extern std::string json_set_var(std::string type, std::vector<std::string> data);
#endif
