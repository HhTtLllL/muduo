// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/Timestamp.h"

#include <sys/time.h>
#include <stdio.h>

/*
      注意:这里为什么要 idndef  然后 有 endif ??  
      在头文件(inttypes.h )中 需要做一个判断    !defined __cplusplus ||  define  __STDC_FORMAT_MACROS  ,
      如果条件成功就 将 PRID 包含进去,    

      因为我们使用的是c++ ,所以第一个条件不成立, 要检查第二个条件
*/
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

using namespace muduo;

//static_assert()    编译时的断言  ,平时使用的 assert 运行时断言
static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp is same size as int64_t");


string Timestamp::toString() const
{
  char buf[32] = {0};
  int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
  int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
  //PRID  是为了跨平台,提高移植性  
  //因为 int64_t  用来表示64位整数,在32位系统中是 long long int ,在 64 为系统中是long int ,所以打印int64_t 的格式化方法
  // %ld     64bit
  // %lld    32bit    PRID 是跨平台的做法, 定义在 inttypes.h  的头文件中
  snprintf(buf, sizeof(buf)-1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
  return buf;
}

//返回一个格式化的字符串
string Timestamp::toFormattedString(bool showMicroseconds) const
{
  char buf[64] = {0};
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
  struct tm tm_time;
  //gmtime 　加了一个　ｒ　表示为线程安全
  //将秒数 转换为结构体
  gmtime_r(&seconds, &tm_time);

  if (showMicroseconds)
  {
    int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
             microseconds);
  }
  else
  {
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
  }
  return buf;
}

Timestamp Timestamp::now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);  //返回一个 timeval 结构体  , NULL 为一个时区
  int64_t seconds = tv.tv_sec;  //获取秒数
  return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);  //tv_usec  微秒
}

