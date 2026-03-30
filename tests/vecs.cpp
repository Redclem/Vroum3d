#include <iostream>
#include <debug.h>
#include <vroum3d.h>

using namespace Vroum3d::Math;
using namespace Vroum3d;

int main(int, char*[])
{
  vec3 v(0);

  std::cout << v << '\n';

  check(v + 1 == vec3(1));
  
  vec3 u(1);

  check(u + u == vec3(2));
  check(3 * u == vec3(3));
  check(u * 5 == vec3(5));

  check(u / 2 == vec3(0.5));

  check(vec3(2) != vec3(3));

  check(!(vec3(3) == vec3(6)));

  check(vec4(4).dot(vec4(4)) == 64);

  vec3 a(1, 2, 2), b(2, 0, 0);
  check(sup(a, b) == vec3(2));

  check(2.0 / vec4(2.0) == vec4(1.0));
  check(2.0 / vec2(1.0, 2.0) == vec2(2.0, 1.0));

  static_assert(vec2::n_comp() == 2);
  static_assert(vec3::n_comp() == 3);
  static_assert(vec4::n_comp() == 4);

  check(vec3(vec4(1.0, 2.0, 3.0, 4.0)) == vec3(1.0, 2.0, 3.0));
  check(vec4(vec3(1.0, 2.0, 3.0)) == vec4(1.0, 2.0, 3.0, 0.0));
  check(vec3(1.0, 2.0, 1.0) / vec3(1.0, 2.0, 2.0) == vec3(1.0, 1.0, 0.5));

  auto [x, y] = vec2(1.0, 2.0);

  return 0;
}
