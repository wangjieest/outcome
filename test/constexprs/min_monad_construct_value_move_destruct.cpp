#include "../../include/boost/spinlock/future.hpp"

extern BOOST_MONAD_NOINLINE int test1()
{
  using namespace boost::spinlock::lightweight_futures;
  monad<int> m1(5);
  monad<int> m2(std::move(m1));
  return std::move(m2).get();
}
extern BOOST_MONAD_NOINLINE void test2()
{
}

int main(void)
{
  int ret=0;
  if(5!=test1()) ret=1;
  test2();
  return ret;
}
