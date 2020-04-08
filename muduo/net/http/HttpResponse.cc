// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "muduo/net/http/HttpResponse.h"
#include "muduo/net/Buffer.h"

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;
//http 响应类封装
void HttpResponse::appendToBuffer(Buffer* output) const
{
  char buf[32];
  //添加相应头
  snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
  output->append(buf);
  output->append(statusMessage_);
  output->append("\r\n");

  if (closeConnection_)
  {
    //如果是短连接,不需要告诉游览器 Content-length ,游览器也能正确处理
   // 短连接 不存在 粘包问题
    output->append("Connection: close\r\n");
  }
  else
  {
    //实体的长度
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    output->append(buf);
    output->append("Connection: Keep-Alive\r\n");
  }
//header 列表
  for (const auto& header : headers_)
  {
    output->append(header.first);
    output->append(": ");
    output->append(header.second);
    output->append("\r\n");
  }
// 头部和 实体 部分应该有一个空行
  output->append("\r\n");   //header  与 body 之间的空行
  output->append(body_);
}
