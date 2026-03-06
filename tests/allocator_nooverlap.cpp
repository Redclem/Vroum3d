


#include <iterator>
#include <random>
#include <set>
#include <spirv/unified1/spirv.h>
#include <vulkan/vulkan_core.h>

#include "../vroum3d/vroum3d.h"

using namespace Vroum3d;

constexpr std::size_t n_blocks = 10000;

struct Segment
{
  VkDeviceMemory mem;
  VkDeviceSize size;
  VkDeviceSize offset;

  bool operator<(const Segment& rhs) const
  {
    return mem < rhs.mem || (mem == rhs.mem && offset < rhs.offset);
  }
};

int VROUM3D_MAIN()
{
	std::ranlux48 rng;
	std::uniform_int_distribution<int> d(0, Allocator::c_log_largest_block_size - 1);

	std::bernoulli_distribution bd;

  Display disp("Allocator test - No Visual");
  Instance inst(disp);

  std::vector<Allocator::allocated_memory_t> blocks;
  std::set<Segment> segms;

	for(std::size_t i = 0; i != n_blocks;++i)
	{
		std::size_t log = d(rng);

		std::size_t size = 1ull << std::size_t(log);

		for(std::size_t i(0); i != log; ++i)
			if(bd(rng))
				size |= 1ull << i;

    auto block = inst.allocate(0, size);
    blocks.push_back(block);

    Segment new_seg{block.memory(), block.size(), block.offset()};
    // Check for overlap in blocks
    
    
    if(auto ub = segms.upper_bound(new_seg); ub != segms.begin())
    {
      ub = std::prev(ub);
      if(ub->mem == new_seg.mem)
        check(ub->size + ub->offset <= new_seg.offset);
    }
    
    if(auto lb = segms.lower_bound(new_seg); lb != segms.end())
    {
      if(lb->mem == new_seg.mem)
        check(lb->offset >= new_seg.offset + new_seg.size);
    }

    segms.insert(new_seg);
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
