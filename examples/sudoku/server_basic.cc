#include "examples/sudoku/sudoku.h"

#include "muduo/base/Atomic.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Thread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpServer.h"

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

//  reactor模型    只有一个IO线程, 这个IO 线程 即负责监听套接字, 也负责已连接套接字
class SudokuServer
{
 public:
  SudokuServer(EventLoop* loop, const InetAddress& listenAddr)
    : server_(loop, listenAddr, "SudokuServer"),
      startTime_(Timestamp::now())
  {
    server_.setConnectionCallback(
        std::bind(&SudokuServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&SudokuServer::onMessage, this, _1, _2, _3));
  }

  void start()
  {
    server_.start();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
        << conn->localAddress().toIpPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp)
  {
    LOG_DEBUG << conn->name();
    size_t len = buf->readableBytes();
    while (len >= kCells + 2)   //反复读取数据， 2  为回车换行字符
    {
      const char* crlf = buf->findCRLF();
      if (crlf)   //如果找到一条完整的请求
      {
        string request(buf->peek(), crlf);    // 取出请求
        buf->retrieveUntil(crlf + 2);      //retrieve 已读取的数据
        
        len = buf->readableBytes();
        
        if (!processRequest(conn, request))  //如果 没找到 找到了 id  部分
        {
          conn->send("Bad Request!\r\n");
          conn->shutdown();
          break;
        }
      }
      else if (len > 100) // id + ":" + kCells + "\r\n"
      {
        conn->send("Id too long!\r\n");
        conn->shutdown();
        break;
      }
      else
      {
        break;
      }
    }
  }

  bool processRequest(const TcpConnectionPtr& conn, const string& request)
  {
    string id;
    string puzzle;
    bool goodRequest = true;

    string::const_iterator colon = find(request.begin(), request.end(), ':');
    if (colon != request.end())
    {
      id.assign(request.begin(), colon); //取出ID
      puzzle.assign(colon+1, request.end());
    }
    else
    {
      puzzle = request;
    }

    if (puzzle.size() == implicit_cast<size_t>(kCells))
    {
      LOG_DEBUG << conn->name();
      string result = solveSudoku(puzzle);
      if (id.empty())
      {
        conn->send(result+"\r\n");
      }
      else
      {
        conn->send(id+":"+result+"\r\n");
      }
    }
    else
    {
      goodRequest = false;
    }
    return goodRequest;
  }

  TcpServer server_;
  Timestamp startTime_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
  EventLoop loop;
  InetAddress listenAddr(9981);
  SudokuServer server(&loop, listenAddr);

  server.start();

  loop.loop();
}

