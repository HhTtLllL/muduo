#include "muduo/net/EventLoop.h"

using namespace muduo;
using namespace muduo::net;

//程序空等
int main()
{
  EventLoop loop;
  loop.loop();
}
