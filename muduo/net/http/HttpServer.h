// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_HTTP_HTTPSERVER_H
#define MUDUO_NET_HTTP_HTTPSERVER_H

#include "muduo/net/TcpServer.h"

namespace muduo
{
namespace net
{

class HttpRequest;
class HttpResponse;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
// http 服务器类封装
class HttpServer : noncopyable
{
 public:
  typedef std::function<void (const HttpRequest&,
                              HttpResponse*)> HttpCallback;

  HttpServer(EventLoop* loop,
             const InetAddress& listenAddr,
             const string& name,
             TcpServer::Option option = TcpServer::kNoReusePort);

  EventLoop* getLoop() const { return server_.getLoop(); }

  /// Not thread safe, callback be registered before calling start().
  void setHttpCallback(const HttpCallback& cb)
  {
    httpCallback_ = cb;
  }

  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void start();

 private:
  void onConnection(const TcpConnectionPtr& conn);
  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime);
  void onRequest(const TcpConnectionPtr&, const HttpRequest&);

// 在应用层使用 http  协议, 在传输层使用Tcp   协议
  TcpServer server_; 

  //当接收到一个 http 请求, 回调 onMessage , onMessage 回调 onRequest , onRequest 回调 httpCallback
  HttpCallback httpCallback_;  //在处理http 请求(即调用onRequest) 的过程中回调此函数,对请求进行具体处理
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_HTTP_HTTPSERVER_H
