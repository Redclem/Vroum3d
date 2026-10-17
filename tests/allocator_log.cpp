
#include "../vroum3d/vroum3d.h"

#include <iterator>
#include <random>

using namespace Vroum3d;
using namespace Vroum3d::Core;

constexpr std::size_t n_test = 10000;

int main(int, char*[])
{
  std::ranlux48 rng;
  std::uniform_int_distribution<int> d(0, 30);

  std::bernoulli_distribution bd;

  for(std::size_t i = 0; i != n_test; ++i)
  {
    std::size_t log = d(rng);

    std::size_t val = 1ull << std::size_t(log);

    for(std::size_t i(0); i != log; ++i)
      if(bd(rng))
        val |= 1ull << i;

    auto res = Allocator::int_log2(val);

    if(res != log)
    {
      std::cout << "Expected log of " << val << " to be " << log << ", got " << res << '\n';
      return 1;
    }
  }

  return 0;
}
