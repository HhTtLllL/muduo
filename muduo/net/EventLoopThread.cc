// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/EventLoopThread.h"

#include "muduo/net/EventLoop.h"

using namespace muduo;
using namespace muduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                 const string& name)
  : loop_(NULL),
    exiting_(false),
                                                                                                              //把this 传递到 threadFunc中
    thread_(std::bind(&EventLoopThread::threadFunc, this), name),
    mutex_(),
    cond_(mutex_),
    callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  if (loop_ != NULL) // not 100% race-free, eg. threadFunc could be running callback_.
  {
    // still a tiny chance to call destructed object, if threadFunc exits just now.
    // but when EventLoopThread destructs, usually programming is exiting anyway.
    loop_->quit(); //退出IO线程, 让IO线程的loop循环退出,从而退出了IO线程
    thread_.join();
  }
}

EventLoop* EventLoopThread::startLoop()
{
  assert(!thread_.started());

  //另外开启一个线程, 调用线程的回调函数
  thread_.start();
  //执行 startloop 线程和 线程的回调 这个线程 是并发执行的,所以没法保证 回调函数和下面的 线程那个先执行
  //  所以 如果 loop == loop 就wait ,回调函数中会给loop 赋值,令它不为 NULL

  EventLoop* loop = NULL;
  {
    MutexLockGuard lock(mutex_);
    while (loop_ == NULL)
    {
      cond_.wait();
    }
    loop = loop_;
  }

  return loop;
}

void EventLoopThread::threadFunc()
{
  EventLoop loop;

  if (callback_)
  {
    callback_(&loop);
  }

  {
    MutexLockGuard lock(mutex_);
    //loop 指针指向了一个栈上的对象,threadFunc 函数退出之后, 这个指针就失效了
    //threadfunc 函数退出后,就意味着线程退出了, 线程对象(  Eventloopthread )也就没有存在的价值,也应该退出
    loop_ = &loop;
    cond_.notify(); // 通知线程
  }

  loop.loop();
  //assert(exiting_);
  MutexLockGuard lock(mutex_);
  loop_ = NULL;
}

