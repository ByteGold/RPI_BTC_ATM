#ifndef OUTPUT_H
#define OUTPUT_H
#define OUTPUT_RUNNING	2
#define OUTPUT_LOW_BTC	3 || (1 << 31)
#define OUTPUT_NO_BTC	3
#define OUTPUT_BACKLOG	4
namespace output{
  void set_var(int output, bool value);
}
#endif
