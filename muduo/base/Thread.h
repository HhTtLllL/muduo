// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include "muduo/base/Atomic.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Types.h"

#include <functional>
#include <memory>
#include <pthread.h>

namespace muduo
{

class Thread : noncopyable
{
 public:
  typedef std::function<void ()> ThreadFunc;

//线程名称具有默认参数
  explicit Thread(ThreadFunc, const string& name = string());
  // FIXME: make it movable in C++11
  ~Thread();

  void start();  //启动线程
  int join(); // return pthread_join()

//是否已经启动
  bool started() const { return started_; } 
  // pthread_t pthreadId() const { return pthreadId_; }
  //线程真实 pid
  pid_t tid() const { return tid_; }
  
  //线程名称
  const string& name() const { return name_; }
//已经启动线程个数
  static int numCreated() { return numCreated_.get(); }

 private:
  void setDefaultName();

  bool       started_; //线程是否已经启动
  bool       joined_;
  pthread_t  pthreadId_;
  pid_t      tid_;  //线程真实 id
  ThreadFunc func_;  //线程回调函数
  string     name_; //线程名称
  CountDownLatch latch_;  //已经创建的线程的个数   原子整数类

  static AtomicInt32 numCreated_;
};

}  // namespace muduo
#endif  // MUDUO_BASE_THREAD_H
