#include "math/mat.hpp"
#include <iostream>
#include <vroum3d.h>

using namespace Vroum3d;
using namespace Vroum3d::Math;

int main()
{
  mat4 A = mat4::scale(vec4{1, 2, 4, 0.5});

  check(A * vec4(1, 0.5, 0.25, 2) == vec4(1));

  A[2][1] = 1.0;

  auto B = translate(vec3(1, 2, 3));

  vec4 v(0, 0, 0, 1);

  check(B * v == vec4(1, 2, 3, 1));

  check((A * B) * v == A * (B * v));

  return 0;
}
