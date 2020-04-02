// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include "muduo/base/Mutex.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/Channel.h"

namespace muduo
{
namespace net
{

class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
//TimerQueue 数据结构的选择,能快速根据当前时间 找到 已经到期的定时器, 也要高效的添加和删除Timer ,因而可以用
// 二叉搜索树, 用 map或者set
class TimerQueue : noncopyable
{
 public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  //添加一个定时器, 返回一个外部类, 共外部使用
  //  一定是线程安全的,可以跨线程调用. 通常情况下被其他线程调用
  TimerId addTimer(TimerCallback cb,
                   Timestamp when,
                   double interval);
// 取消一个定时器,
  void cancel(TimerId timerId);

 private:

  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  // This requires heterogeneous comparison lookup (N3465) from C++14
  // so that we can find an T* in a set<unique_ptr<T>>.

  /*
       无法得到指向同一对象的两个unique_ptr  指针
       但可以进行移动构造与移动赋值操作, 即所有权可以移动到另一个对象 (而非拷贝构造函数)
  */
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TimerList;  //按照事件戳来排序  -- Timestamp
  typedef std::pair<Timer*, int64_t> ActiveTimer;
  typedef std::set<ActiveTimer> ActiveTimerSet;
/*
 一下成员函数 只可能在其所属的 I/O 线程中调用, 因而不必加锁
 服务器的性能杀手之一是锁竞争 , 所以要尽可能少用锁
*/
  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerId timerId);


  // called when timerfd alarms
  // 是一个回调函数, 
  void handleRead();  
  // move out all expired timers

  //返回超时的定时器列表
  std::vector<Entry> getExpired(Timestamp now);
  //对超时的定时器 重置
  void reset(const std::vector<Entry>& expired, Timestamp now);

  bool insert(Timer* timer);

  EventLoop* loop_;    //所属的 EventLoop
  const int timerfd_;
  Channel timerfdChannel_;
  // Timer list sorted by expiration
  TimerList timers_;   //timers_ 是按到期时间排序的

  // for cancel()
   /*
          timers_ 和 activeTimers_ 保存的是相同的数据

          timers_ 是按照到期时间排序, activeTimers_ 是按照对象地址排序
  */
  ActiveTimerSet activeTimers_;
  bool callingExpiredTimers_; /* atomic */  //是否处于调用 处理超时定时器中
  ActiveTimerSet cancelingTimers_;  //保存的是被取消的定时器
};

}  // namespace net
}  // namespace muduo
#endif  // MUDUO_NET_TIMERQUEUE_H
