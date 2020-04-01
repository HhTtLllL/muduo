// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include "muduo/base/Mutex.h"
#include "muduo/base/noncopyable.h"

#include <pthread.h>

namespace muduo
{
  /*
        应用程序设计中有必要提供线程私有的全局变量,尽在某个线程中有效,但却可以跨多个函数访问

        posix 线程库通过维护一定的数据结构来解决这个问题,这些数据称为 tsd    Thread-specific Data
        线程特定数据也称为线程本地存储 tls   Thread-local storage

        对于POD 类型的线程本地存储,可以用 __thread 关键字

       pthread_key_create 
       pthread_key_delete
       pthread_getspecific
       pthread_setspecific 


  */

template<typename T>
class ThreadLocal : noncopyable
{
 public:
  ThreadLocal()
  {
    MCHECK(pthread_key_create(&pkey_, &ThreadLocal::destructor));
  }

  ~ThreadLocal()
  {
    //这里只是销毁 key ,并不是销毁实际数据, 实际数据使用 创建是的回调销毁
    MCHECK(pthread_key_delete(pkey_));
  }

//获取实际数据
  T& value()
  {                                                                                               // 通过key 来获取数据
    T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));
    if (!perThreadValue)
    {
      T* newObj = new T();
      MCHECK(pthread_setspecific(pkey_, newObj));
      perThreadValue = newObj;
    }
    return *perThreadValue;
  }

 private:

//调用pthread_key_create  时需要传递一个回调,用这个回调函数删除实际的数据
  static void destructor(void *x)
  {
    T* obj = static_cast<T*>(x);
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy; (void) dummy;
    delete obj;
  }

 private: 
 //实际的数据
  pthread_key_t pkey_;
};

}  // namespace muduo

#endif  // MUDUO_BASE_THREADLOCAL_H
