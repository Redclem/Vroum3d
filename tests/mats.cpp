#include "math/mat.hpp"
#include <iostream>
#include <vroum3d.h>

using namespace Vroum3d;
using namespace Vroum3d::Math;

int main()
{
  mat4 A = mat4::scale(vec4{1, 2, 4, 0.5});

  check(A * vec4(1, 0.5, 0.25, 2) == vec4(1));

  std::cout << A << '\n';

  std::cout << translate(vec3(1, 2, 3)) << '\n';

  return 0;
}
