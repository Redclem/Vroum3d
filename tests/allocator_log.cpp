
#include <iterator>
#include <random>

#include "../vroum3d/vroum3d.h"

using namespace Vroum3d;

constexpr std::size_t n_test = 10000;

int VROUM3D_MAIN()
{
	std::ranlux48 rng;
	std::uniform_int_distribution<int> d(0, 30);

	std::bernoulli_distribution bd;

	for(std::size_t i = 0; i != n_test;++i)
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
