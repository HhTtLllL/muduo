#include "muduo/base/Timestamp.h"
#include <vector>
#include <stdio.h>

using muduo::Timestamp;

void passByConstReference(const Timestamp& x)
{
  printf("%s\n", x.toString().c_str());
}

void passByValue(Timestamp x)
{
  printf("%s\n", x.toString().c_str());
}

void benchmark()
{
  //静态常量前 加一个 k  ,谷歌编程规范
  const int kNumber = 1000*1000;

  std::vector<Timestamp> stamps;
  stamps.reserve(kNumber); //预留这么多空间
  //插入100万个 时间  
  for (int i = 0; i < kNumber; ++i)
  {
    stamps.push_back(Timestamp::now());
  }
  //计算所耗费的时间  --时间的消耗主要浪费在 gettimeofday 中, 因为之前已经预留了 空间
  printf("%s\n", stamps.front().toString().c_str());
  printf("%s\n", stamps.back().toString().c_str());
  //插入100 个元素所耗费的时间
  printf("%f\n", timeDifference(stamps.back(), stamps.front()));

  int increments[100] = { 0 };

  int64_t start = stamps.front().microSecondsSinceEpoch();
  for (int i = 1; i < kNumber; ++i)
  {
    int64_t next = stamps[i].microSecondsSinceEpoch();
    int64_t inc = next - start;
    start = next;
    if (inc < 0)
    {
      printf("reverse!\n");
    }
    else if (inc < 100)
    {
      ++increments[inc];
    }
    else
    {
      printf("big gap %d\n", static_cast<int>(inc));
    }
  }

//输出 小于 100微秒的时间的 次数
  for (int i = 0; i < 100; ++i)
  {
    printf("%2d: %d\n", i, increments[i]);
  }
}

int main()
{
  Timestamp now(Timestamp::now());
  printf("%s\n", now.toString().c_str());
  passByValue(now);  //值传递
  passByConstReference(now);  //引用传递
  benchmark();
}

