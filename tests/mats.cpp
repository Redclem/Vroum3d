#include "math/mat.hpp"
#include <cmath>
#include <iostream>
#include <vroum3d.h>

using namespace Vroum3d;
using namespace Vroum3d::Math;

int main(int, char*[])
{
  {
    mat4 A = mat4::scale(vec4{1, 2, 4, 0.5});

    check(A * vec4(1, 0.5, 0.25, 2) == vec4(1));

    A[2][1] = 1.0;

    auto B = translate(vec3(1, 2, 3));

    vec4 v(0, 0, 0, 1);

    check(B * v == vec4(1, 2, 3, 1));

    check((A * B) * v == A * (B * v));

    mat4 C = A;
    C *= B;
    check(C == A * B);
  }

  {
    vec4 x(1, 1, 1, 1);

    auto P = proj<float>(M_PI_2, 1, 0.1, 100.0);
    auto T = translate(vec3(0, 0, -3));

    auto A = P * T;

    auto itm = T * x;
    auto px2 = P * itm;
    check(0.0 <= px2.z / px2.w && px2.z /px2.w <= 1);

    auto px = A * x;
    check(0.0 <= px.z / px.w && px.z / px.w <= 1.0);
  }

  check(mat3::scale(vec2(0.5)) * vec3(2.0) == vec3(1.0, 1.0, 2.0));
  check(vec3(2.0) * mat3::scale(vec2(0.5)) == vec3(1.0, 1.0, 2.0));

  check(mat3(mat4::translate(vec3(1.0, 2.0, 3.0))) == mat3::identity());

  {
    auto P = proj<float>(M_PI_2, 1, 0.1, 100.0);

    check(P != transpose(P));
    check(P == transpose(transpose(P)));
    vec4 x(1, 1, 1, 1);

    check(x * transpose(P) == P * x);
  }

  return 0;
}
