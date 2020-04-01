// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"

namespace muduo
{
//1.  可以用于所有子线程等待主线程发起"   起跑    "    主线程用 condition  通知 所有子线程 
//2.  也可以用于主线程等待子线程初始化完毕才开始工作
class CountDownLatch : noncopyable
{
 public:

  explicit CountDownLatch(int count);

  void wait();

  void countDown(); //计数器 - 1 , 当计数器减为0 后发起通知

  int getCount() const;  //获取当前计数器的值

 private:
 //用 mutable 修饰,表示 getcount ()成员函数 可以 改变这个 变量的状态,虽然加了 const ,但是不能改变 count_ 的状态
  mutable MutexLock mutex_;
  Condition condition_ GUARDED_BY(mutex_);
  int count_ GUARDED_BY(mutex_);   //计数器
};

}  // namespace muduo
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
