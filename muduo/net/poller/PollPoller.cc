// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/poller/PollPoller.h"

#include "muduo/base/Logging.h"
#include "muduo/base/Types.h"
#include "muduo/net/Channel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

PollPoller::PollPoller(EventLoop* loop)
  : Poller(loop)
{
}

PollPoller::~PollPoller() = default;

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
  // XXX pollfds_ shouldn't change
  int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
  int savedErrno = errno;
  Timestamp now(Timestamp::now());
  if (numEvents > 0)
  {
    LOG_TRACE << numEvents << " events happened";
    fillActiveChannels(numEvents, activeChannels); //将事件 放到通道里面
  }
  else if (numEvents == 0)
  {
    LOG_TRACE << " nothing happened";
  }
  else
  {
    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      LOG_SYSERR << "PollPoller::poll()";
    }
  }
  return now;
}

void PollPoller::fillActiveChannels(int numEvents,
                                    ChannelList* activeChannels) const
{
  for (PollFdList::const_iterator pfd = pollfds_.begin();
      pfd != pollfds_.end() && numEvents > 0; ++pfd)
  {
    if (pfd->revents > 0)
    {
      --numEvents;
      ChannelMap::const_iterator ch = channels_.find(pfd->fd);
     
      assert(ch != channels_.end());
      Channel* channel = ch->second;
    
      assert(channel->fd() == pfd->fd);
      channel->set_revents(pfd->revents);
      // pfd->revents = 0;
      activeChannels->push_back(channel);
    }
  }
}
//用于注册或者更新 事件,注册某一个文件描述符的事件,可读,可写
void PollPoller::updateChannel(Channel* channel)
{
  //要在IO 线程中调用才可以(所属线程)
  Poller::assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
  if (channel->index() < 0)
  {
    //index <0 说明是一个新通道
    // a new one, add to pollfds_
    assert(channels_.find(channel->fd()) == channels_.end()); //在事件列表中找不到
    
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);
    int idx = static_cast<int>(pollfds_.size())-1;
    channel->set_index(idx);
    channels_[pfd.fd] = channel;
  }
  else
  {
    //更新一个已经存在的 事件
    // update existing one
    //判断这个事件确实存在
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));

    struct pollfd& pfd = pollfds_[idx];
    assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    //将一个通道暂时更改为不关注事件,但是不从Poller  中移除该通道
    if (channel->isNoneEvent())
    {
      // ignore this pollfd
      //暂时忽略该文件描述符的事件
      //这里pfd.fd 可以直接设置为 -1
      
      //这里将设为负数,因为负数不是一个合法的文件描述符,这里为什么要 -1 ,有一个特殊的文件描述符, 0 .
      pfd.fd = -channel->fd()-1;  //这样子设置是为了 removeChannel 优化   ,
    }
  }
}
//从数组中移除
void PollPoller::removeChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd();
  assert(channels_.find(channel->fd()) != channels_.end());
  assert(channels_[channel->fd()] == channel); //能找到它
  //如果想要移除一个通道,一定是不在关注事件了, 所以需要现将事件设为 不关注. 这里才能断言成功
  assert(channel->isNoneEvent());
  int idx = channel->index();

  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));

  const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
  
  assert(pfd.fd == -channel->fd()-1 && pfd.events == channel->events());
  
  size_t n = channels_.erase(channel->fd());//从map 中移除 ,用key 移除
  
  assert(n == 1); (void)n;
  
  //如果是最后一个通道,则直接移除
  if (implicit_cast<size_t>(idx) == pollfds_.size()-1) 
  {
    pollfds_.pop_back();
  }
  //如果不是最后一个通道,则将它于最后一个通道交换数据,然后将最后一个移除
  else
  {
    int channelAtEnd = pollfds_.back().fd; //没有交换之前最后一个通道的文件描述符
    iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
    if (channelAtEnd < 0)
    {
      channelAtEnd = -channelAtEnd-1;
    }
    channels_[channelAtEnd]->set_index(idx); //改写在数组索引
    pollfds_.pop_back();
  }
}

