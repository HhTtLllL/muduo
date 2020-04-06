// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"

namespace muduo
{
namespace net
{

class EventLoop;
/*
      任何一个线程,只要创建并群星了 EventLoop ,都称之为 IO 线程
       IO 线程不一定是主线程

       muduo并发模型 one loop per thread + threadpool 
      
      为了方便今后使用,定义了 EventLoopThread 类, 该类封装了 IO 线程
        
*/
class EventLoopThread : noncopyable
{
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const string& name = string());
  ~EventLoopThread();

 //启动线程,调用thread_ 中的start() ,start 启动 threadfunc . 该线程就成为了IO线程
  EventLoop* startLoop();

 private:
 //线程回调函数
  void threadFunc();

//loop_  指向一个eventloop 对象
  EventLoop* loop_ GUARDED_BY(mutex_);
  //是否退出
  bool exiting_;

  Thread thread_;   //包含了一个thread 类的对象  -- 基于对象的思想
  MutexLock mutex_;
  Condition cond_ GUARDED_BY(mutex_);

  //如果回调函数不是空的, 回调函数在loop 事件循环之前被调用, 相当于初始化,初始化之后在进行事件循环
  ThreadInitCallback callback_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_EVENTLOOPTHREAD_H

