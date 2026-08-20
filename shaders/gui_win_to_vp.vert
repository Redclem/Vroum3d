#version 450

layout(location=0) in uvec2 i_pos;

layout(push_constant) uniform Pc 
{
	vec2 twice_inv_size;
	vec4 color;
} pc;

void main()
{
	gl_Position = vec4(i_pos * pc.twice_inv_size - 1.0, 0.0, 1.0);
}
