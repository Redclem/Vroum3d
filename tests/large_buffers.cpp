#include <random>
#include <vroum3d.h>

using namespace Vroum3d::Core;

using namespace Vroum3d;

constexpr std::size_t n_buffers = 10;

int main(int, char*[])
{
  std::ranlux48 rng;
  std::uniform_int_distribution<int> d(0, Instance::c_largest_block_size);

  std::bernoulli_distribution bd;

  Instance inst;

  std::vector<Buffer> blocks;

  for(std::size_t i = 0; i != n_buffers; ++i) { blocks.emplace_back(inst, Instance::c_largest_block_size + d(rng), 0, 0); }

  while(!blocks.empty())
  {
    auto b_idx = std::uniform_int_distribution<std::size_t>(0, blocks.size() - 1)(rng);

    auto block = std::exchange(blocks[b_idx], std::move(blocks.back()));

    blocks.pop_back();
  }

  return 0;
}
