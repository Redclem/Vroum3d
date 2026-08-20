#version 450

layout(location = 0) out vec4 o_color;

layout(push_constant) uniform Pc 
{
	vec2 twice_inv_size;
	vec4 color;
} pc;

void main()
{
	o_color = pc.color;
}
