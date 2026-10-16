#version 450

layout(location = 0) out vec2 o_pos;

vec2 positions[3] = {
  vec2(-1.0, -1.0),
  vec2(3.0, -1.0),
  vec2(-1.0, 3.0)
};

void main()
{
  o_pos = positions[gl_VertexIndex];
  gl_Position = vec4(positions[gl_VertexIndex], 0.5, 1.0);
}
