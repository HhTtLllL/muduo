// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_HTTP_HTTPCONTEXT_H
#define MUDUO_NET_HTTP_HTTPCONTEXT_H

#include "muduo/base/copyable.h"

#include "muduo/net/http/HttpRequest.h"

namespace muduo
{
namespace net
{

class Buffer;
// http 协议解析类
class HttpContext : public muduo::copyable
{
 public:
 //正在处于 那个状态 
 /*
    1. 正在处于解析请求行状态
    2. 正在处于解析请求头状态
    3. 状态处于解析 实体状态
    4.全部解析完毕
 */
  enum HttpRequestParseState
  {
    kExpectRequestLine,
    kExpectHeaders,
    kExpectBody,
    kGotAll,
  };

  HttpContext()
    : state_(kExpectRequestLine)  //初始状态
  {
  }

  // default copy-ctor, dtor and assignment are fine

  // return false if any error
  bool parseRequest(Buffer* buf, Timestamp receiveTime);

  bool gotAll() const
  { return state_ == kGotAll; }
 //重置 httpcontext 状态
  void reset()
  {
    state_ = kExpectRequestLine;
    HttpRequest dummy;
    request_.swap(dummy);
  }

  const HttpRequest& request() const
  { return request_; }

  HttpRequest& request()
  { return request_; }

 private:
  bool processRequestLine(const char* begin, const char* end);

  HttpRequestParseState state_;  //请求解析状态
  HttpRequest request_;   //http 请求
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_HTTP_HTTPCONTEXT_H
