// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include "muduo/base/Types.h"

namespace muduo
{
namespace CurrentThread
{
  // internal
  /*
  __thread 修饰的变量是线程局部存储的

  如果不带__thread,这些变量就是全局变量,多个线程是共享全局变量的

  但是如果用__thread修饰, 也是全局变量,不过只是该线程的全局变量, 每个线程都会自己创建一份
  
    __thread 只能用于 POD类型(与c 兼容的原始数据)
用户自定义的构造函数或者虚函数的类不是 

如果想要修饰用户自定义的构造函数  则需要使用 tsd

boost::is_same   判断两个两种类型是否为 同一种类型
  */
  extern __thread int t_cachedTid;  //线程真实pid(tid) 缓存,  每次调用函数比较费时,减少系统调用的次数
  extern __thread char t_tidString[32];  //字符串形式的 tid (格式化为字符串)
  extern __thread int t_tidStringLength;  //字符串线程形式的长度
  extern __thread const char* t_threadName;   //线程名称
  void cacheTid();

  inline int tid()
  {
    //先判断是否为 0  ,如果为0 就代表没有缓存过,如果不为零就代表已经缓存过,
    //直接调用缓存
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
      cacheTid();
    }
    return t_cachedTid;
  }

  inline const char* tidString() // for logging
  {
    return t_tidString;
  }

  inline int tidStringLength() // for logging
  {
    return t_tidStringLength;
  }

  inline const char* name()
  {
    return t_threadName;
  }

  bool isMainThread();

  void sleepUsec(int64_t usec);  // for testing

  string stackTrace(bool demangle);
}  // namespace CurrentThread
}  // namespace muduo

#endif  // MUDUO_BASE_CURRENTTHREAD_H
