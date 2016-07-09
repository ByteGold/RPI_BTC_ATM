#ifndef UTIL_H
#define UTIL_H
#define PRINT_DEBUG 0
#define PRINT_NOTICE 1
#define PRINT_WARNING 2
#define PRINT_ERROR 3
#define PRINT_CRITICAL 4
#define P_DEBUG PRINT_DEBUG
#define P_NOTICE PRINT_NOTICE
#define P_WARN PRINT_WARNING
#define P_ERR PRINT_ERROR
#define P_CRIT PRINT_CRITICAL
#include "exception"
#include "cstring"
#include "iostream"
#include "execinfo.h"
extern int search_for_argv(std::string);
extern void print(std::string data, int level);
#define LOCK_RUN(a, b) a.lock();b;a.unlock();
#endif
