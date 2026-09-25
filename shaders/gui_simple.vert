#version 450

layout(location=0) in vec2 i_pos;

layout(push_constant) uniform Pc 
{
	vec4 color;
	vec2 twice_inv_size;
  uint texture_index;
} pc;

void main()
{
	gl_Position = vec4(i_pos * pc.twice_inv_size - 1.0, 0.0, 1.0);
}
