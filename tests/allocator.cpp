

#include <iterator>
#include <random>

#include "../vroum3d/vroum3d.h"

using namespace Vroum3d;

constexpr std::size_t n_blocks = 10000;

int VROUM3D_MAIN()
{
	std::ranlux48 rng;
	std::uniform_int_distribution<int> d(0, Allocator::c_log_largest_block_size - 1);

	std::bernoulli_distribution bd;

  std::uniform_int_distribution alignd(0, 8);

  Display disp("Allocator test - No Visual");
  Instance inst(disp);

  std::vector<Allocator::allocated_memory_t> blocks;

	for(std::size_t i = 0; i != n_blocks;++i)
	{
		std::size_t log = d(rng);

		std::size_t size = 1ull << std::size_t(log);

    auto align = 1 << alignd(rng);

		for(std::size_t i(0); i != log; ++i)
			if(bd(rng))
				size |= 1ull << i;

    auto new_block = inst.allocate(0, size, align);
    check(new_block.offset() % align == 0);

    check(new_block.size() >= size);

    blocks.push_back(new_block);
	}

  while(!blocks.empty())
  {
     auto b_idx = std::uniform_int_distribution<std::size_t>(0, blocks.size() - 1)(rng);

     auto block = blocks[b_idx];
     blocks[b_idx] = blocks.back();

     blocks.pop_back();

     inst.free(block);
  }

	return 0;
}
