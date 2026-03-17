


#include <iterator>
#include <random>

#include <chrono>

#include "../vroum3d/vroum3d.h"

using namespace Vroum3d;

constexpr std::size_t n_blocks = 1000000;

int VROUM3D_MAIN()
{
	std::ranlux48 rng;
	std::uniform_int_distribution<int> d(0, Allocator::c_log_largest_block_size - 2);

	std::bernoulli_distribution bd;

  std::uniform_int_distribution alignd(0, 8);

  Display disp("Allocator test - No Visual");
  DisplayInstance inst(disp);

  std::vector<Allocator::allocated_memory_t> blocks;

  typedef std::chrono::high_resolution_clock::duration duration_t;
  duration_t tot_alloc_time(0), tot_free_time(0);

	for(std::size_t i = 0; i != n_blocks;++i)
	{
		std::size_t log = d(rng);

		std::size_t size = 1ull << std::size_t(log);

    auto align = 1 << alignd(rng);

		for(std::size_t i(0); i != log; ++i)
			if(bd(rng))
				size |= 1ull << i;

    auto start = std::chrono::high_resolution_clock::now();

    auto new_block = inst.allocate(0, size, align);

    auto end = std::chrono::high_resolution_clock::now();
    tot_alloc_time += end - start;

    blocks.push_back(new_block);
	}

  while(!blocks.empty())
  {
     auto b_idx = std::uniform_int_distribution<std::size_t>(0, blocks.size() - 1)(rng);

     auto block = blocks[b_idx];
     blocks[b_idx] = blocks.back();

     blocks.pop_back();

    auto start = std::chrono::high_resolution_clock::now();

    inst.free(block);

    auto end = std::chrono::high_resolution_clock::now();

    tot_free_time += end - start;
  }

  std::cout << "Avg alloc time : " << std::chrono::duration_cast<std::chrono::nanoseconds>(tot_alloc_time).count() / n_blocks << std::endl;
  std::cout << "Avg free time : " << std::chrono::duration_cast<std::chrono::nanoseconds>(tot_free_time).count() / n_blocks << std::endl;

	return 0;
}
