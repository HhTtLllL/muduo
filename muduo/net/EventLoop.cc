// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/EventLoop.h"

#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/net/Channel.h"
#include "muduo/net/Poller.h"
#include "muduo/net/SocketsOps.h"
#include "muduo/net/TimerQueue.h"

#include <algorithm>

#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
  /*
    one loop per thread 以为着每个线程只能有一个Eventloop　对象，以后在创建就可以检查这个变量，如果已经赋值就说明当前线程已经创建过EventLoop对象了

    线程调用静态函数EventLoop::getEventLoopOfCurrentThread就可以获得当前线程的EventLoop对象的指针了
  */
 //当前线程Eventloop　对象指针
 //线程局部存储
 //__thread 表示每个线程都有这个指针变量
__thread EventLoop* t_loopInThisThread = 0;  

const int kPollTimeMs = 10000;

int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return evtfd;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
 public:
  IgnoreSigPipe()
  {
    ::signal(SIGPIPE, SIG_IGN);
    // LOG_TRACE << "Ignore SIGPIPE";
  }
};
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe initObj;
}  // namespace

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}

EventLoop::EventLoop()
  : looping_(false),    //是否进入了loop循环
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    iteration_(0),
    threadId_(CurrentThread::tid()),  //将当前创建对象的线程ID初始化给　该对象
    poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),  //创建一个eventfd
    wakeupChannel_(new Channel(this, wakeupFd_)), //创建一个通道,将wakefd 传进来
    currentActiveChannel_(NULL)
{
  //记录日志
  LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_; 
  //检查当前线程是否创建了eventloop 对象
  if (t_loopInThisThread) //如果该线程已经创建了eventloop　对象
  {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_;
  }
  else
  {
    t_loopInThisThread = this;
  }
  wakeupChannel_->setReadCallback(
      std::bind(&EventLoop::handleRead, this));
  // we are always reading the wakeupfd
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
            << " destructs in thread " << CurrentThread::tid();
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  ::close(wakeupFd_);
  t_loopInThisThread = NULL;
}
//事件循环，该函数不能跨线程调用,只能在创建该对象的线程中调用
//事件循环必须在IO线程执行,因此 loop 会检查 pre-condition
void EventLoop::loop()
{
  assert(!looping_); //判断之前是否已经调用过 loop
 
 //断言当前处于创建该对象的线程中
  assertInLoopThread(); 
  
  looping_ = true;
  quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
  LOG_TRACE << "EventLoop " << this << " start looping";

  while (!quit_)
  {
    activeChannels_.clear(); //清空激活事件集合

    //调用poll  得到活动的通道
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_); //pool_wait 或 epool_wait 阻塞在这里,  调用poll返回活动的通道
    ++iteration_;
    if (Logger::logLevel() <= Logger::TRACE)
    {
      printActiveChannels();//打印活动的通道 日志
    }
    // TODO sort channel by priority   按照优先级对事件集合排序
    eventHandling_ = true; //将是否正在处理事件循环标志设为true
    //遍历活动通道进行处理,  - - -- - - -处理事件
    for (Channel* channel : activeChannels_)
    {
      currentActiveChannel_ = channel;   //设置当前活动通道
      currentActiveChannel_->handleEvent(pollReturnTime_);  //处理活动    事件处理 I/O
    }
    currentActiveChannel_ = NULL;
    eventHandling_ = false;     //将是否正在处理事件循环标志设为false

    //  执行一些  其他线程或者是当前线程 添加的 用户任务,让IO 线程也能执行一些用户任务
    doPendingFunctors();
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::quit()
{
  quit_ = true;
  // There is a chance that loop() just executes while(!quit_) and exits,
  // then EventLoop destructs, then we are accessing an invalid object.
  // Can be fixed using mutex_ in both places.
  if (!isInLoopThread())
  {
    wakeup();
  }
}
//在 I/O 线程中执行某个回调函数,该函数可以跨线程调用
void EventLoop::runInLoop(Functor cb)
{
  if (isInLoopThread())
  {
    //如果是当前IO线程 调用runInLoop,则同步调用cb
    cb();
  }
  else
  {
    //如果是其他线程调用 runInLoop,则异步地将cb添加到队列中
    // 让EventLoop 所对应的IO  线程 执行相应的 回调函数函数
    queueInLoop(std::move(cb));
  }
}
// 将对应的  回调任务添加到 队列中
void EventLoop::queueInLoop(Functor cb)
{

  {
  MutexLockGuard lock(mutex_);
  pendingFunctors_.push_back(std::move(cb)); //添加到任务队列中
  }


  //调用 queueInLoop 的线程不是当前IO线程需要唤醒,一遍IO线程可以及时的处理这个任务

  //或者调用queueInLoop 的线程是当前I O 线程,并且此时正在调用 pending functor ,需要唤醒
  //只有当前IO线程的事件回调中调用 queueInLoop 才不需要唤醒

  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}

size_t EventLoop::queueSize() const
{
  MutexLockGuard lock(mutex_);
  return pendingFunctors_.size();
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
{
  return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
  return timerQueue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  if (eventHandling_)
  {
    assert(currentActiveChannel_ == channel ||
        std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
  }
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}
// 用来唤醒IO 线程, 当其他线程需要终止 当前IO 线程的时候, 
// 需要 先唤醒 这个IO 线程,  
// 回调在   创建这个 EventLoop 的时候设置
//   当其他线程 需要 quit 这个线程的时候, IO 线程可能正在 阻塞在poll ,此时,
// wakeup 往 fd 中写数据, 触发回调的可写事件 , 然后取处理
void EventLoop::wakeup()
{
  uint64_t one = 1;
  //唤醒另一个线程 往这个线程中写入 8 个字节,就可以唤醒这个线程
  ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}
// 从唤醒的 wakeup 线程中 读出数据
void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::doPendingFunctors()//执行任务队列中回调函数的任务
{
  // 先定义一个空的  functors
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;//是否正在调用pendingFunctors_的函数对象。

  {
  MutexLockGuard lock(mutex_);

  // swap 调用完之后, pending 中就为空,  
  functors.swap(pendingFunctors_);
  }
    //循环调用  回调函数
  for (const Functor& functor : functors)
  {
    functor();
  }
  
  callingPendingFunctors_ = false;


}

void EventLoop::printActiveChannels() const
{
  for (const Channel* channel : activeChannels_)
  {
    LOG_TRACE << "{" << channel->reventsToString() << "} ";
  }
}

