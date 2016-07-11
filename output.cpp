/*
  This file controls output to the user (LED, 16x2). Most debugging information
  isn't sent out through these means, but some things the user should know are.
 */
#include "main.h"
#include "gpio.h"
#include "output.h"
#include "json_rpc.h"

void output::set_var(int output, bool value){
  // true = in, false = out
  switch(output){
  case OUTPUT_RUNNING:
    gpio::set_dir(OUTPUT_RUNNING, value);
    break;
  case OUTPUT_LOW_BTC:
    gpio::set_dir(OUTPUT_LOW_BTC, value);
    break;
  case OUTPUT_NO_BTC:
    gpio::set_dir(OUTPUT_NO_BTC, value);
    break;
  case OUTPUT_BACKLOG:
    gpio::set_dir(OUTPUT_BACKLOG, value);
    break;
  }
}
