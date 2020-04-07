// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H

#include "muduo/base/noncopyable.h"
#include "muduo/net/InetAddress.h"

#include <functional>
#include <memory>

namespace muduo
{
namespace net
{

class Channel;
class EventLoop;

//主动发起连接,带有自动重连的功能

class Connector : noncopyable,
                  public std::enable_shared_from_this<Connector>
{
 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;

  Connector(EventLoop* loop, const InetAddress& serverAddr);
  ~Connector();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  void start();  // can be called in any thread
  void restart();  // must be called in loop thread
  void stop();  // can be called in any thread

  const InetAddress& serverAddress() const { return serverAddr_; }

 private:
  enum States { kDisconnected, kConnecting, kConnected };
  static const int kMaxRetryDelayMs = 30*1000; // 30 秒,最大重连延迟时间
  static const int kInitRetryDelayMs = 500;  //0.5 秒,初始状态,连接不上,0.5 秒后重连

  void setState(States s) { state_ = s; }
  void startInLoop();
  void stopInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWrite();
  void handleError();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

  EventLoop* loop_;  //所属EventLoop
  InetAddress serverAddr_;   //服务器端地址
  bool connect_; // atomic   是否连接
  States state_;  // FIXME: use atomic variable
  std::unique_ptr<Channel> channel_;  //Connector 对应的通道
  NewConnectionCallback newConnectionCallback_;   //连接成功回调函数
  int retryDelayMs_;   //重连延迟时间
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_CONNECTOR_H
