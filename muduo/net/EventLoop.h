// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <atomic>
#include <functional>
#include <vector>

#include <boost/any.hpp>

#include "muduo/base/Mutex.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/TimerId.h"

namespace muduo
{
namespace net
{

class Channel;
class Poller;
class TimerQueue;

///
/// Reactor, at most one per thread. 每一个线程最多有一个
///
/// This is an interface class, so don't expose too much details.
//这是一个接口类，不需要暴露太多细节
// 一个 eventloop  包含多个 channel  (聚合关系)
class EventLoop : noncopyable
{
 public:
  typedef std::function<void()> Functor;

  EventLoop();
  ~EventLoop();  // force out-line dtor, for std::unique_ptr members.

  /// Loops forever.
  /// Must be called in the same thread as creation of the object.
  ////必须在创建对象的同一线程中调用
  void loop();

  /// Quits loop.
  ///
  /// This is not 100% thread safe, if you call through a raw pointer,
  /// better to call through shared_ptr<EventLoop> for 100% safety.
  void quit();

  ///
  /// Time when poll returns, usually means data arrival.
  ///
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  int64_t iteration() const { return iteration_; }

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.
  void runInLoop(Functor cb);
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  void queueInLoop(Functor cb);

  size_t queueSize() const;

  // timers

  ///
  /// Runs callback at 'time'.
  /// Safe to call from other threads.
  ///
  // 在某个时刻运行定时器,  调用 TimeerQueue 中的 addTimer
  TimerId runAt(Timestamp time, TimerCallback cb);
  ///
  /// Runs callback after @c delay seconds.
  /// Safe to call from other threads.
  ///
  //过一段时间运行定时器  -- 调用 TimeerQueue 中的 addTimer
  TimerId runAfter(double delay, TimerCallback cb);
  ///
  /// Runs callback every @c interval seconds.
  /// Safe to call from other threads.
  ///每隔一段时间运行定时器  ---  调用 TimeerQueue 中的 addTimer
  TimerId runEvery(double interval, TimerCallback cb);
  ///
  /// Cancels the timer.
  /// Safe to call from other threads.
  ///
  //取消定时器 -- 调用Timerqueue 的cancel 
  void cancel(TimerId timerId);

  // internal usage
  void wakeup();
  void updateChannel(Channel* channel);      //从Poller 中添加(注册)或者更新通道（事件）
  void removeChannel(Channel* channel);      //从Poller中移除通道
  bool hasChannel(Channel* channel);

  // pid_t threadId() const { return threadId_; }
  void assertInLoopThread()   //判断当前线程是否在I/O 线程中
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }
  //判断当前线程是否为创建该对象的线程　　CurrentThread::tid() 获取当前线程
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  // bool callingPendingFunctors() const { return callingPendingFunctors_; }
  bool eventHandling() const { return eventHandling_; }

  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const
  { return context_; }

  boost::any* getMutableContext()
  { return &context_; }

  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();
  void handleRead();  // waked up
  void doPendingFunctors();

  void printActiveChannels() const; // DEBUG

  typedef std::vector<Channel*> ChannelList;

  bool looping_; /* atomic */   //是否正在执行loop循环
  std::atomic<bool> quit_;  //是否已经调用quit()　函数退出loop 循环
  bool eventHandling_; /* atomic */    //enentHandling 是否正在处理event 事件
  bool callingPendingFunctors_; /* atomic */    //是否正在调用pendingFunctors_ 的函数对象
  int64_t iteration_;
  const pid_t threadId_;  //记录当前对象属于那个线程  --当前对象所属的线程ID
  Timestamp pollReturnTime_;  //调用poll 函数返回的时间戳
  std::unique_ptr<Poller> poller_;    //用来调用poll 或者 epool    生成期由EventLoop 控制
  std::unique_ptr<TimerQueue> timerQueue_;
  int wakeupFd_;    // 用于eventfd    创建一个文件描述符,用于事件通知
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  std::unique_ptr<Channel> wakeupChannel_; //wakeup 通道, 该通道将会纳入poller_ 来管理
  boost::any context_;

  // scratch variables
  ChannelList activeChannels_;   //记录这激活事件的集合　　　　Poller 返回的活动通道
  Channel* currentActiveChannel_;  //当前正在处理的channel 事件　　当前正在处理的活动通道

  mutable MutexLock mutex_;
  std::vector<Functor> pendingFunctors_  GUARDED_BY(mutex_);  //是当前线程要执行的任务的集合
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
