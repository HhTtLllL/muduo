#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"
#include "muduo/base/Timestamp.h"

#include <vector>
#include <stdio.h>

using namespace muduo;
using namespace std;

MutexLock g_mutex;
vector<int> g_vec;
const int kCount = 10*1000*1000;

void threadFunc()
{
  for (int i = 0; i < kCount; ++i)
  {
    MutexLockGuard lock(g_mutex);
    g_vec.push_back(i);
  }
}

int foo() __attribute__ ((noinline));

int g_count = 0;
int foo()
{
  MutexLockGuard lock(g_mutex);
  if (!g_mutex.isLockedByThisThread())
  {
    printf("FAIL\n");
    return -1;
  }

  ++g_count;
  return 0;
}

int main()
{
  MCHECK(foo());
  if (g_count != 1)
  {
    printf("MCHECK calls twice.\n");
    abort();
  }

//最多 8 个线程
  const int kMaxThreads = 8;
  //预留空间
  g_vec.reserve(kMaxThreads * kCount);
//等级一个时间戳
  Timestamp start(Timestamp::now());
  for (int i = 0; i < kCount; ++i)
  {
    g_vec.push_back(i);
  }

  printf("single thread without lock %f\n", timeDifference(Timestamp::now(), start));

//再次登记 时间 
  start = Timestamp::now();

  threadFunc();
  printf("single thread with lock %f\n", timeDifference(Timestamp::now(), start));

  for (int nthreads = 1; nthreads < kMaxThreads; ++nthreads)
  {
    std::vector<std::unique_ptr<Thread>> threads;
    g_vec.clear();
    start = Timestamp::now();
    for (int i = 0; i < nthreads; ++i)
    {
      threads.emplace_back(new Thread(&threadFunc));
      threads.back()->start();
    }
    for (int i = 0; i < nthreads; ++i)
    {
      threads[i]->join();
    }
    printf("%d thread(s) with lock %f\n", nthreads, timeDifference(Timestamp::now(), start));
  }
}

