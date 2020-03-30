#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpServer.h"

using namespace muduo;
using namespace muduo::net;

/*
      接受新连接，在1079端口侦听新连接，接受连接之后什么都不做，程序空等
      muduo 会自动丢弃收到的数据
*/
int main()
{
  EventLoop loop;
  TcpServer server(&loop, InetAddress(1079), "Finger");
  server.start();
  loop.loop();
}
