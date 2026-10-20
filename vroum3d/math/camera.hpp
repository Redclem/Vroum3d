#ifndef VROUM3D_MATH_CAMERA_HPP_INCLUDED
#define VROUM3D_MATH_CAMERA_HPP_INCLUDED

#include "mat.hpp"
#include "vec.hpp"

namespace Vroum3d::Math
{

/** FPS Style camera */
class Camera {
public:
  Camera(vec3 position = {0}, float yaw = 0, float pitch = 0) : m_position(position), m_yaw(yaw), m_pitch(pitch) {}

  void add_position(const auto &dpos) { m_position += dpos; }

  void add_yaw(const auto &dyaw) { m_yaw += dyaw; }

  void add_pitch(const auto &dpitch) { m_pitch = std::clamp<float>(m_pitch + dpitch, -M_PI_2, M_PI_2); }

  /** Get camera transform. No projection.
   *
   * World space -> Cam space
   * */
  mat4 get_matrix() const { return scale(vec4(-1.0, -1.0, 1.0, 1.0)) * mat4(rotate<0>(m_pitch) * rotate<1>(m_yaw)) * translate(-m_position); }

  /** Get inverse camera transform.
   *
   * Cam Space -> World Space
   * */
  mat4 get_inv_matrix() const { return scale(vec4(-1.0, -1.0, 1.0, 1.0)) * translate(m_position) * mat4(rotate<1>(-m_yaw) * rotate<0>(-m_pitch)); }

  mat3 get_inv_matrix_orient_only() const { return rotate<1>(-m_yaw) * rotate<0>(-m_pitch); }

  const auto &position() const { return m_position; }

protected:
  vec3 m_position;
  float m_yaw;
  float m_pitch;
};

} // namespace Vroum3d::Math

#endif
