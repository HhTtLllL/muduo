// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_EXCEPTION_H
#define MUDUO_BASE_EXCEPTION_H

#include "muduo/base/Types.h"
#include <exception>

namespace muduo
{
/*
  backtrace  栈回溯,保存各个栈帧的地址
  backtrace_symbols ,根据地址,转成相应的函数符号
*/

class Exception : public std::exception
{
 public:
 //在构造函数中 登记栈回溯信息
  Exception(string what);
  ~Exception() noexcept override = default;

  // default copy-ctor and operator= are okay.

//返回message
  const char* what() const noexcept override
  {
    return message_.c_str();
  }

//返回stack
  const char* stackTrace() const noexcept
  {
    return stack_.c_str();
  }

 private:
  string message_;  //用来保存异常信息的字符串
  string stack_;      //用来保存异常发生的时候栈的信息
};

}  // namespace muduo

#endif  // MUDUO_BASE_EXCEPTION_H
