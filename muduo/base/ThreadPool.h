// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"
#include "muduo/base/Types.h"

#include <deque>
#include <vector>

namespace muduo
{
// 固定线程池, 线程池中的线程是固定的
class ThreadPool : noncopyable
{
 public:
  typedef std::function<void ()> Task;

  explicit ThreadPool(const string& nameArg = string("ThreadPool"));
  ~ThreadPool();

  // Must be called before start().
  void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
  void setThreadInitCallback(const Task& cb)
  { threadInitCallback_ = cb; }
//启动线程, 启动的个数是固定的
  void start(int numThreads);

  //关闭线程池
  void stop();

  const string& name() const
  { return name_; }

  size_t queueSize() const;

  // Could block if maxQueueSize > 0
  // There is no move-only version of std::function in C++ as of C++14.
  // So we don't need to overload a const& and an && versions
  // as we do in (Bounded)BlockingQueue.
  // https://stackoverflow.com/a/25408989
  //运行任务, 往线程池中 任务队列添加任务
  void run(Task f);

 private:
  bool isFull() const REQUIRES(mutex_);
  //线程池的线程执行的任务
  void runInThread();

  //获取任务, 线程池中的线程需要获取任务,然后执行任务
  Task take();

  mutable MutexLock mutex_;
  //唤醒线程队列中的线程 执行任务
  Condition notEmpty_ GUARDED_BY(mutex_);
  Condition notFull_ GUARDED_BY(mutex_);
  //线程池的名称
  string name_;
  Task threadInitCallback_;
  //线程队列, 内部存放的是线程的指针
  std::vector<std::unique_ptr<muduo::Thread>> threads_;
  //任务队列, 
  std::deque<Task> queue_ GUARDED_BY(mutex_);
  size_t maxQueueSize_;
  //表示线程池是否在运行中
  bool running_;
};

}  // namespace muduo

#endif  // MUDUO_BASE_THREADPOOL_H
