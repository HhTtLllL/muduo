// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <functional>

#include "muduo/net/Channel.h"
#include "muduo/net/Socket.h"

namespace muduo
{
namespace net
{

class EventLoop;
class InetAddress;
 /*
    主要功能是: socket ,bind ,listen 

    一般来说,在上层应用程序中,我们不直接使用 Acceptor ,而是把它作为 TcoServer 的成员


    当有了新的用户连接的时候, acceptor 有一个监听套接字, 这个套接字处于可读状态 ,然后 channel  会观察这个套接字的
    可读事件,channel 处于活跃的通道, poller 将它返回 回来,调用handevent , handevent 调用 acceptor  中的 handleread,
    handleRead 调用 accept 建立连接, 并且回调 new connection  函数

*/
///
/// Acceptor of incoming TCP connections.
//  用于 accept(2)   接受TCP 连接
class Acceptor : noncopyable
{
 public:
  typedef std::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

  Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  bool listenning() const { return listenning_; }
  void listen();

 private:
  void handleRead();

  EventLoop* loop_;
  Socket acceptSocket_;  //监听套接字  --即 server socket
  Channel acceptChannel_;  //会观察  这个 监听套接字的 (  acceptSocket)的可读事件
                                                          //用于观察此 socket 的 readable 事件,并回调accptor::handleRead(),
                                                        //handleRead() 调用 accept() 来接受新连接,并回调用户callback()  应用层的回调 
  //用户设置的回调函数
  NewConnectionCallback newConnectionCallback_;
  bool listenning_;
  int idleFd_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_ACCEPTOR_H
