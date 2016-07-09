#ifndef DRIVER_H
#define DRIVER_H
#include "functional"
#include "mutex"
class driver_t{
 public:
  int count;
  std::mutex lock;
  std::function<int(int*)> init;
  std::function<int()> close;
  std::function<int(int*)> run;
  driver_t();
  ~driver_t();
  
};
#endif
