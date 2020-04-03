// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/AsyncLogging.h"
#include "muduo/base/LogFile.h"
#include "muduo/base/Timestamp.h"

#include <stdio.h>

using namespace muduo;

AsyncLogging::AsyncLogging(const string& basename,
                           off_t rollSize,
                           int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    rollSize_(rollSize),
    thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
    latch_(1),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_()
{
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  buffers_.reserve(16);
}
//前端每生成一条日志消息的时候会调用 append()
void AsyncLogging::append(const char* logline, int len)
{
  muduo::MutexLockGuard lock(mutex_);
  //如果当前缓冲区剩余空间足够大 ( 可以容纳下当前日志的大小 ),则会直接把日志消息拷贝(追加)到当前缓冲中
  //这里拷贝一条日志消息 并不会带来多大的开销
  //
  if (currentBuffer_->avail() > len)
  {
    currentBuffer_->append(logline, len);
  }
  else
  {
    //否则,说明当前缓冲已经写满, 就把它送入(移入) buffers_,
    //并试图把预备好的另一块缓冲 (nextbuffer_)  移用(move) 为当前缓冲
    buffers_.push_back(std::move(currentBuffer_));

    if (nextBuffer_)
    {
      currentBuffer_ = std::move(nextBuffer_);
    }
    else
    {
      //如果前端 写入速度太快,一下子把两块缓冲都用完了, 那么只好分配一块新的buffer,作为当前缓冲
      currentBuffer_.reset(new Buffer); // Rarely happens
    }

    //然后追加日志消息并通知(唤醒) 后端开始写入日志数据
    currentBuffer_->append(logline, len);
    cond_.notify();
  }
}

void AsyncLogging::threadFunc()
{
  assert(running_ == true);

  latch_.countDown();
  
  LogFile output(basename_, rollSize_, false);
  
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  //缓冲区清零
  newBuffer1->bzero();
  newBuffer2->bzero();
  
  BufferVector buffersToWrite;
  //提前分配 16 个空间
  buffersToWrite.reserve(16);

  while (running_)
  {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      muduo::MutexLockGuard lock(mutex_);
      if (buffers_.empty())  // unusual usage!
      {
        cond_.waitForSeconds(flushInterval_);
      }
      //先将当前缓冲移入buffers_ 中
      buffers_.push_back(std::move(currentBuffer_));
      //令当前空间的newbuffer1 移为当前缓冲
      currentBuffer_ = std::move(newBuffer1);

      //新创建的 write vector , 和已经写满的 vector buffers_ ,交换空间
      //内部交换指针,而非复制
      buffersToWrite.swap(buffers_);

      if (!nextBuffer_)
      {
        //替换缓冲区,这样保证 前端 始终有一个预备 buffer 可供调配.
        nextBuffer_ = std::move(newBuffer2);
      }
    }

    assert(!buffersToWrite.empty());

    if (buffersToWrite.size() > 25)
    {
      char buf[256];
      snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               buffersToWrite.size()-2);
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
    }

    for (const auto& buffer : buffersToWrite)
    {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffer->data(), buffer->length());
    }

    if (buffersToWrite.size() > 2)
    {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);
    }

    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}

