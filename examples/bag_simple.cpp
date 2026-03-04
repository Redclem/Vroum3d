#include "../vroum3d/vroum3d.h"

#include <vector>
#include <iostream>
#include <random>

using namespace Vroum3d;



int VROUM3D_MAIN()
{
  std::ranlux48 rng;

  
  typedef decltype(rng()) elem_t;
  typedef Bag<elem_t> bag_t;
  bag_t b;
  
  constexpr std::size_t n_elems = bag_t::c_subbag_size * 2;
  
  std::vector<bag_t::ptr_t> ptrs;

  for(int i = 0; i != n_elems; ++i)
  {
    auto newelem = b.allocate();
    ptrs.push_back(newelem);
  }

  for(int i = 0; i != n_elems; i += 2)
    b.release(ptrs[i]);

  for(int i = 0; i != n_elems; i += 2)
    ptrs[i] = b.allocate();

  elem_t sum(0);
  for(int i = 0; i != n_elems; ++i)
  {
    *ptrs[i] = rng();
    sum += *ptrs[i];
  }

  elem_t sum2(0);
  for(int i = 0; i != n_elems; ++i)
    sum2 += *ptrs[i];

  if(sum2 == sum)
    std::cout << "Sum ok\n";
  else
  {
    std::cout << "Sum not ok\n";
    return 1;
  }

  if(b.n_subbags() != 2)
  {
    std::cout << "Overallocation\n";
    return 1;
  }

  return 0;
}
