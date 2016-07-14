#ifndef DRIVER_H
#define DRIVER_H
#include "functional"
#include "mutex"
class driver_t{
 public:
  int count;
  std::mutex lock;
  std::function<int()> init;
  std::function<int()> close;
  std::function<int(int*)> run;
  std::function<int()> reject_all;
  std::function<int()> accept_all;
  driver_t();
  ~driver_t();
};
#endif
