// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_ATOMIC_H
#define MUDUO_BASE_ATOMIC_H

#include "muduo/base/noncopyable.h"

#include <stdint.h>

namespace muduo
{

namespace detail
{

  //模仿java中原子操作的类
template<typename T>
class AtomicIntegerT : noncopyable
{
 public:
  AtomicIntegerT()
    : value_(0)
  {
  }
//将 构造和 赋值设为私有的

  // uncomment if you need copying and assignment
  //
  // AtomicIntegerT(const AtomicIntegerT& that)
  //   : value_(that.get())
  // {}
  //
  // AtomicIntegerT& operator=(const AtomicIntegerT& that)
  // {
  //   getAndSet(that.get());
  //   return *this;
  // }

//原子性的比较和设置
//比较当前value的值是否等于 第一个 0 ,如果等于 0 ,就把value 设为 第二个 0 , 返回的是 修改之前的 值
//线程 安全
  T get()
  {
    // in gcc >= 4.7: __atomic_load_n(&value_, __ATOMIC_SEQ_CST)
    return __sync_val_compare_and_swap(&value_, 0, 0);
  }
//先获取,然后在 进行 +  的值,返回的是没有修改的值
  T getAndAdd(T x)
  {
    // in gcc >= 4.7: __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST)
    return __sync_fetch_and_add(&value_, x);
  }

//先加后获取,返回的是修改之后的值
  T addAndGet(T x)
  {
    return getAndAdd(x) + x;
  }
// 自增
  T incrementAndGet()
  {
    return addAndGet(1);
  }
//自减
  T decrementAndGet()
  {
    return addAndGet(-1);
  }
//先获取,后加
  void add(T x)
  {
    getAndAdd(x);
  }
//自加,先加后获取
  void increment()
  {
    incrementAndGet();
  }
//自减,先减后获取
  void decrement()
  {
    decrementAndGet();
  }
//返回原来的值,然后设为新值
  T getAndSet(T newValue)
  {
    // in gcc >= 4.7: __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST)
    return __sync_lock_test_and_set(&value_, newValue);
  }

 private:
  volatile T value_;
};
}  // namespace detail
// 32 和 64 的整数类
typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef detail::AtomicIntegerT<int64_t> AtomicInt64;

}  // namespace muduo

#endif  // MUDUO_BASE_ATOMIC_H
