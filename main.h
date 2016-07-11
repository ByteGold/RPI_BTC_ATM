#ifndef MAIN_H
#define MAIN_H
#ifndef __linux
#error This program is for Linux only
#endif
#include "iostream"
#include "cstring"
#include "thread"
#include "vector"
#include "chrono"
#include "signal.h"
#include "unistd.h"
#define CURRENCY_USD 1 // only supported one
extern bool running;
extern int argc;
extern char** argv;
#endif
