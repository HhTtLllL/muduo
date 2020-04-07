// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/EventLoopThreadPool.h"

#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg)
  : baseLoop_(baseLoop),
    name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
  // Don't delete loop, it's stack variable
}

// 启动线程池
void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
  assert(!started_);
  baseLoop_->assertInLoopThread();

  started_ = true;
// 创建若干个线程
  for (int i = 0; i < numThreads_; ++i)
  {
    char buf[name_.size() + 32];
    snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
    EventLoopThread* t = new EventLoopThread(cb, buf);
    
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    //启动EventLoopThread 线程,在进入事件循环之前,会调用 cb
    // 并且将返回的 EventLoop 对象的指针压入 loops
    loops_.push_back(t->startLoop());
  }
  if (numThreads_ == 0 && cb)
  {
    //如果只有一个EventLoop ,在这个 EventLoop 进入事件循环之前,调用cb
    cb(baseLoop_);
  }
}

//一个新的连接到来,需要选择一个 EventLoop 处理
EventLoop* EventLoopThreadPool::getNextLoop()
{
  baseLoop_->assertInLoopThread();
  assert(started_);
  // 这个 baseloop_  是 acceptor(监听套接字所属的 EventLoop), 处理连接客户端的 main_eventloop
  EventLoop* loop = baseLoop_;

//如果 loops_ 为空,也就上是面这个函数没有创建出线程(numThreads == 0),则 loops_ 指向 baseLoop_ 
//如果不为空,按照round-robin(RR 轮叫) 的调度方式选择一个EventLoop
  if (!loops_.empty())
  {
    // round-robin
    loop = loops_[next_];
    ++next_;
    if (implicit_cast<size_t>(next_) >= loops_.size())
    {
      next_ = 0;
    }
  }

  //如果为空,就代表 一共就只有一个线程 main_loop, 就返回这个main_loop
  return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
  baseLoop_->assertInLoopThread();
  EventLoop* loop = baseLoop_;

  if (!loops_.empty())
  {
    loop = loops_[hashCode % loops_.size()];
  }
  return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
  baseLoop_->assertInLoopThread();
  assert(started_);
  if (loops_.empty())
  {
    return std::vector<EventLoop*>(1, baseLoop_);
  }
  else
  {
    return loops_;
  }
}
