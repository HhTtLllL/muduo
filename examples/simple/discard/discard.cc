#include "examples/simple/discard/discard.h"

#include "muduo/base/Logging.h"

using namespace muduo;
using namespace muduo::net;
// bind 可以绑定
/*
      functions
      functions objects
      member functions,_1 必须是某个object　地址
      data members,_1        必须是某个object 地址

*/
DiscardServer::DiscardServer(EventLoop* loop,
                             const InetAddress& listenAddr)
  : server_(loop, listenAddr, "DiscardServer")
{
  server_.setConnectionCallback(
      std::bind(&DiscardServer::onConnection, this, _1));
  server_.setMessageCallback(
      std::bind(&DiscardServer::onMessage, this, _1, _2, _3));
}

void DiscardServer::start()
{
  server_.start();
}

void DiscardServer::onConnection(const TcpConnectionPtr& conn)
{
  LOG_INFO << "DiscardServer - " << conn->peerAddress().toIpPort() << " -> "
           << conn->localAddress().toIpPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
}

void DiscardServer::onMessage(const TcpConnectionPtr& conn,
                              Buffer* buf,
                              Timestamp time)
{
  string msg(buf->retrieveAllAsString());   //接受不做任何处理
  LOG_INFO << conn->name() << " discards " << msg.size()
           << " bytes received at " << time.toString();
}

