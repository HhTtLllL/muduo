// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "muduo/net/Buffer.h"
#include "muduo/net/http/HttpContext.h"

using namespace muduo;
using namespace muduo::net;

bool HttpContext::processRequestLine(const char* begin, const char* end)
{
  bool succeed = false;
  const char* start = begin;
  const char* space = std::find(start, end, ' ');  //查找空格所在位置
  if (space != end && request_.setMethod(start, space))  //解析请求方法
  {
    start = space+1;
    space = std::find(start, end, ' ');  
    if (space != end)
    {
      const char* question = std::find(start, space, '?');  
      if (question != space)
      {
        request_.setPath(start, question);  //解析路径
        request_.setQuery(question, space);
      }
      else
      {
        request_.setPath(start, space);
      }
      start = space+1;
      succeed = end-start == 8 && std::equal(start, end-1, "HTTP/1.");
      if (succeed)
      {
        if (*(end-1) == '1')
        {
          request_.setVersion(HttpRequest::kHttp11);
        }
        else if (*(end-1) == '0')
        {
          request_.setVersion(HttpRequest::kHttp10);
        }
        else
        {
          succeed = false;
        }
      }
    }
  }
  return succeed;
}

// return false if any error
// 解析请求
bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime)
{
  bool ok = true;
  bool hasMore = true;

  //这里相当于是一个状态机
  while (hasMore)
  {
    if (state_ == kExpectRequestLine)   //处于解析请求行状态
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        ok = processRequestLine(buf->peek(), crlf);   //解析请求行
        if (ok)
        {
          request_.setReceiveTime(receiveTime);   //设置请求事件
          buf->retrieveUntil(crlf + 2);   //将请求行从buf 中取回 包括 \r\n
          state_ = kExpectHeaders;   //httpContext 将状态改为 kExpectHeaders
        }
        else
        {
          hasMore = false;
        }
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectHeaders)  //解析header 
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        const char* colon = std::find(buf->peek(), crlf, ':');  //查找 冒号 所在位置
        if (colon != crlf)
        {
          request_.addHeader(buf->peek(), colon, crlf);
        }
        else
        {
          // empty line, end of header
          // FIXME:
          state_ = kGotAll;  // httpContext 将状态改为 kGotAll
          hasMore = false;
        }
        buf->retrieveUntil(crlf + 2);  //将header 从buf 中取回, 包括\r\n
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectBody)  //当前还不支持请求中 带 body
    {
      // FIXME:
    }
  }
  return ok;
}
