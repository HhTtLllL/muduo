// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_TIMESTAMP_H
#define MUDUO_BASE_TIMESTAMP_H

#include "muduo/base/copyable.h"
#include "muduo/base/Types.h"

#include <boost/operators.hpp>

namespace muduo
{

///
/// Time stamp in UTC, in microseconds resolution.
///
/// This class is immutable.
/// It's recommended to pass it by value, since it's passed in register on x64.
//对象有两种语义,    -
// 值语义 : 可以拷贝,拷贝之后与原对象脱离关系
//  对象语义: 要么是不能拷贝的,要么是可以拷贝的,拷贝之后与原对象仍然存在一定的关系,
//比如共享底层资源(要实现自己的拷贝构造函数)

//            copyable    是一个空基类,标识类, 标识凡是继承copyable 的类 都是  值类型 ,可以拷贝的
class Timestamp : public muduo::copyable,
                  public boost::equality_comparable<Timestamp>,
                  public boost::less_than_comparable<Timestamp>
                  // less_than_comparable  --只需要实现 < ,可以自动实现(推倒)    >,  <=  ,>= .
{
 public:
  ///
  /// Constucts an invalid Timestamp.
  ///
  Timestamp()
    : microSecondsSinceEpoch_(0)
  {
  }

  ///
  /// Constucts a Timestamp at specific time
  ///
  /// @param microSecondsSinceEpoch
  explicit Timestamp(int64_t microSecondsSinceEpochArg)
    : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
  {
  }

  void swap(Timestamp& that)
  {
    std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
  }

  // default copy/assignment/dtor are Okay

  string toString() const;
  string toFormattedString(bool showMicroseconds = true) const;

  bool valid() const { return microSecondsSinceEpoch_ > 0; }

  // for internal usage.
  int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
  time_t secondsSinceEpoch() const
  { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }

  ///
  /// Get time of now.
  ///
  // 获取当前时间
  static Timestamp now();
  //获取一个无效的时间
  static Timestamp invalid()
  {
    return Timestamp();
  }

  static Timestamp fromUnixTime(time_t t)
  {
    return fromUnixTime(t, 0);
  }

  static Timestamp fromUnixTime(time_t t, int microseconds)
  {
    return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
  }
//  一微秒 等于 百万分之1 秒, 1000*1000  等于一百万
  static const int kMicroSecondsPerSecond = 1000 * 1000;

 private:
 //linux 和 类unix 从 1970-01-01  00:00:00  开始计时     86400 秒
  int64_t microSecondsSinceEpoch_;  //用来记录时间(微秒数)
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

///
/// Gets time difference of two timestamps, result in seconds.
///
/// @param high, low
/// @return (high-low) in seconds
/// @c double has 52-bit precision, enough for one-microsecond
/// resolution for next 100 years.
//计算两个时间的差
inline double timeDifference(Timestamp high, Timestamp low)
{
  int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
  return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

///
/// Add @c seconds to given timestamp.
///
/// @return timestamp+seconds as Timestamp
///
//在一个时间的基础上 加上 多少秒
inline Timestamp addTime(Timestamp timestamp, double seconds)
{
  //现将秒 转化为 微秒
  int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);  
  return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}  // namespace muduo

#endif  // MUDUO_BASE_TIMESTAMP_H
