// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_HTTP_HTTPREQUEST_H
#define MUDUO_NET_HTTP_HTTPREQUEST_H

#include "muduo/base/copyable.h"
#include "muduo/base/Timestamp.h"
#include "muduo/base/Types.h"

#include <map>
#include <assert.h>
#include <stdio.h>

namespace muduo
{
namespace net
{
//http请求类的封装
class HttpRequest : public muduo::copyable
{
 public:
  enum Method
  {
    //kInvaild   无效的方法, 其他为 当前支持的方法
    kInvalid, kGet, kPost, kHead, kPut, kDelete
  };
  //http 协议版本
  enum Version
  {
    kUnknown, kHttp10, kHttp11
  };

  HttpRequest()
    : method_(kInvalid),
      version_(kUnknown)
  {
  }
// 设置请求版本
  void setVersion(Version v)
  {
    version_ = v;
  }
//  获取请求版本
  Version getVersion() const
  { return version_; }
 
 //设置方法
  bool setMethod(const char* start, const char* end)
  {
    assert(method_ == kInvalid);
    string m(start, end);
    if (m == "GET")
    {
      method_ = kGet;
    }
    else if (m == "POST")
    {
      method_ = kPost;
    }
    else if (m == "HEAD")
    {
      method_ = kHead;
    }
    else if (m == "PUT")
    {
      method_ = kPut;
    }
    else if (m == "DELETE")
    {
      method_ = kDelete;
    }
    else
    {
      method_ = kInvalid;
    }
    return method_ != kInvalid;
  }

  Method method() const
  { return method_; }


//返回 请求放阿飞
  const char* methodString() const
  {
    const char* result = "UNKNOWN";
    switch(method_)
    {
      case kGet:
        result = "GET";
        break;
      case kPost:
        result = "POST";
        break;
      case kHead:
        result = "HEAD";
        break;
      case kPut:
        result = "PUT";
        break;
      case kDelete:
        result = "DELETE";
        break;
      default:
        break;
    }
    return result;
  }
// 设置路径
  void setPath(const char* start, const char* end)
  {
    path_.assign(start, end);
  }
//返回路径
  const string& path() const
  { return path_; }

  void setQuery(const char* start, const char* end)
  {
    query_.assign(start, end);
  }

  const string& query() const
  { return query_; }

//设置接受时间
  void setReceiveTime(Timestamp t)
  { receiveTime_ = t; }
  //返回接受时间
  Timestamp receiveTime() const
  { return receiveTime_; }
//添加一个头部信息
  void addHeader(const char* start, const char* colon, const char* end)
  {
    string field(start, colon);  //header 域
    ++colon;
    //取出左空格
    while (colon < end && isspace(*colon))
    {
      ++colon;
    }
    string value(colon, end);   //header 值
    //取出右空格
    while (!value.empty() && isspace(value[value.size()-1]))
    {
      value.resize(value.size()-1);
    }
    headers_[field] = value;
  }

// 获取头部信息
  string getHeader(const string& field) const
  {
    string result;
    std::map<string, string>::const_iterator it = headers_.find(field);
    if (it != headers_.end())
    {
      result = it->second;
    }
    return result;
  }

  const std::map<string, string>& headers() const
  { return headers_; }
//  交换数据成员
  void swap(HttpRequest& that)
  {
    std::swap(method_, that.method_);
    std::swap(version_, that.version_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    receiveTime_.swap(that.receiveTime_);
    headers_.swap(that.headers_);
  }

 private:
  Method method_;   //请求方法
  Version version_;    //协议版本 1.0/1.1
  string path_;               //请求路径
  string query_;                    
  Timestamp receiveTime_;    //请求时间 
  std::map<string, string> headers_;   //header 列表
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_HTTP_HTTPREQUEST_H
