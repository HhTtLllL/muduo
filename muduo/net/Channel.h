// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include "muduo/base/noncopyable.h"
#include "muduo/base/Timestamp.h"

#include <functional>
#include <memory>

namespace muduo
{
namespace net
{
//是对 IO 注册与响应的封装

class EventLoop;

///
/// A selectable I/O channel.
/// 负责注册与相应IO事件，它不拥有文件描述符
/*
    channel 是 acceptor ,Connector,EventLoop,timeQueue,tcpConnection 的成员，生命期有后者控制
    Channel 的生命周期不是由EventLoop 控制，因为它俩的关系是聚合关系



    channel 不拥有 文件描述符,当它销毁时 ,不调用close  关闭文件描述符
*/
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,  文件描述符可能被socket  拥有
/// an eventfd, a timerfd, or a signalfd
class Channel : noncopyable
{
 public:
  typedef std::function<void()> EventCallback;   //事件的回调处理
  typedef std::function<void(Timestamp)> ReadEventCallback; //读事件的回调 ,多了一个时间戳

  Channel(EventLoop* loop, int fd);
  ~Channel();
//对所发生的IO 事件的处理
//有可读事件来, 调用channel 的 handeventl , 然后 handleEvent 调用acceptor 的 handleRead
  void handleEvent(Timestamp receiveTime);
  
  //回调函数的注册
  void setReadCallback(ReadEventCallback cb)
  { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb)
  { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb)
  { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb)
  { errorCallback_ = std::move(cb); }

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void tie(const std::shared_ptr<void>&);

  int fd() const { return fd_; }  //channel 对应的文件描述符
  int events() const { return events_; }    //channel 注册的事件
  void set_revents(int revt) { revents_ = revt; } // used by pollers
  // int revents() const { return revents_; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }  //判断事件是否属于   没有事件

//关注读的事件,或者加入这个事件,
/*updata  调用EventLoop 的  updateChannel  ,updateChannel 调用 poller的 updateChannel ,把事件注册 到 poller中*/
  void enableReading() { events_ |= kReadEvent; update(); }
  void disableReading() { events_ &= ~kReadEvent; update(); }
  void enableWriting() { events_ |= kWriteEvent; update(); }
  void disableWriting() { events_ &= ~kWriteEvent; update(); }

  //不关注事件了
  void disableAll() { events_ = kNoneEvent; update(); }
  
  bool isWriting() const { return events_ & kWriteEvent; }
  bool isReading() const { return events_ & kReadEvent; }

  // for Poller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  // for debug
  //将事件转为string ,以便调试
  string reventsToString() const; 
  string eventsToString() const;

  void doNotLogHup() { logHup_ = false; }

  EventLoop* ownerLoop() { return loop_; }
  void remove();

 private:
  static string eventsToString(int fd, int ev);
//当调用 update ,update 调用 EventLoop 的 updateChannel    ,然后eventloop 调用 poller 的 updateChannel 
//相当于将 channel  注册到 poller ,或者将 channel 的 fd (文件描述符)的可读可写事件注册到 poller 中
  void update(); //负责注册或者更新 IO 的可读和可写事件
  void handleEventWithGuard(Timestamp receiveTime);

  static const int kNoneEvent;  //没有事件   0
  static const int kReadEvent;   //可读事件或者紧急事件 POLLIN / POLLPRI (tcp的带外数据)
  static const int kWriteEvent;  // POLLOUT

  EventLoop* loop_;   //记录这个对象所属的EventLoop
  const int  fd_;     //文件描述符,但不负责关闭该文件描述符
  int        events_;  //关注的事件,由用户设置
  int        revents_; // it's the received event types of epoll or poll             
                                //  poll/epoll实际返回的事件  目前活动的事件,由Eventloop/Poller设置
  int        index_; // used by Poller.  表示在poll的事件数组中的序号
  bool       logHup_;

  std::weak_ptr<void> tie_;
  bool tied_;
  bool eventHandling_;  //是否处于事件处理中
  bool addedToLoop_;

  //回调
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_CHANNEL_H
