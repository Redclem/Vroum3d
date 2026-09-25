#version 450

layout(location = 0) out vec4 o_color;

layout(push_constant) uniform Pc 
{
	vec4 color;
	vec2 twice_inv_size;
  uint texture_index;
} pc;

void main()
{
	o_color = pc.color;
}
