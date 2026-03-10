

#include <iterator>
#include <random>
#include <vulkan/vulkan_core.h>

#include "../vroum3d/vroum3d.h"

using namespace Vroum3d;

constexpr std::size_t n_blocks = 10000;

int VROUM3D_MAIN()
{
	std::ranlux48 rng;
	std::uniform_int_distribution<int> d(0, 24);

	std::bernoulli_distribution bd;

  std::uniform_int_distribution alignd(0, 8);

  Display disp("Allocator test - No Visual");
  Instance inst(disp);

  std::vector<Allocator::owned_memory_t> blocks;

	for(std::size_t i = 0; i != n_blocks;++i)
	{
		std::size_t log = d(rng);

		std::size_t size = 1ull << std::size_t(log);

    auto align = 1 << alignd(rng);

		for(std::size_t i(0); i != log; ++i)
			if(bd(rng))
				size |= 1ull << i;

    Allocator::owned_memory_t new_block;

    try {
      new_block = inst.allocate(0, size, align);
         
    } catch (const VulkanError& ve) {
      if(ve == VK_ERROR_OUT_OF_DEVICE_MEMORY) break;
      else throw ve;
    }

      check(new_block.offset() % align == 0);

      check(new_block.size() >= size);

      blocks.push_back(std::move(new_block));
	}

  while(!blocks.empty())
  {
     auto b_idx = std::uniform_int_distribution<std::size_t>(0, blocks.size() - 1)(rng);

     auto block = std::exchange(blocks[b_idx], std::move(blocks.back()));

     blocks.pop_back();

     inst.free(block);
  }

	return 0;
}
