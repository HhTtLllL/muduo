// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_POLLER_EPOLLPOLLER_H
#define MUDUO_NET_POLLER_EPOLLPOLLER_H

#include "muduo/net/Poller.h"

#include <vector>

struct epoll_event;

namespace muduo
{
namespace net
{
  /*
     epoll 使用LT 模式的原因
    
      1.与 poller 兼容

      2.LT模式不会发生漏掉事件的BUG,但是POLLOUT事件 不能一开始就关注,否则会发生bufy loop,
      而应该在write 无法完全写入内核缓冲区的时候才关注,将未写入内核缓冲区的数据添加到应用层 output buffer ,
      直到应用层 output buffer 写完,停止关注 POLLOUT 事件
      
      3.  读写的时候不必等候 EAGAIN,可以节省系统调用次数,降低延迟.  
      注: 如果用ET 模式,读的时候读到 EAGAIN,写的时候直到 output buffer 写完或者EAGAIN
  
  */

///
/// IO Multiplexing with epoll(4).
///
class EPollPoller : public Poller
{
 public:
  EPollPoller(EventLoop* loop);
  ~EPollPoller() override;

  Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
  void updateChannel(Channel* channel) override;
  void removeChannel(Channel* channel) override;

 private:
  static const int kInitEventListSize = 16;

  static const char* operationToString(int op);

  void fillActiveChannels(int numEvents,
                          ChannelList* activeChannels) const;
  void update(int operation, Channel* channel);

  typedef std::vector<struct epoll_event> EventList;

  int epollfd_;
  EventList events_;
};

}  // namespace net
}  // namespace muduo
#endif  // MUDUO_NET_POLLER_EPOLLPOLLER_H
