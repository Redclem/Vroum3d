#version 450

layout(location=0) in vec2 i_pos;
layout(location=1) in vec2 i_uv;

layout(location=0) out vec2 o_uv;

layout(push_constant) uniform Pc 
{
	vec2 twice_inv_size;
} pc;

void main()
{
	gl_Position = vec4(i_pos * pc.twice_inv_size - 1.0, 0.0, 1.0);
	o_uv = i_uv;
}
