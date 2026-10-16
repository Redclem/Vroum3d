#version 450
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable

layout(location = 0) in vec2 i_pos;

layout(location = 0) out vec4 o_color;

layout(location = 0) uniform accelerationStructureEXT topLevelAS;

void main()
{
  o_color = vec4(0.5);
}
