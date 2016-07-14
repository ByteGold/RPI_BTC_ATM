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
#define DEFAULT_THREAD_SLEEP 10
extern bool running;
extern int argc;
extern char** argv;
#endif
