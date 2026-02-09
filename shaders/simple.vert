
#version 450

layout(location=0) in vec3 pos;

layout(location=0) out vec3 outpos;


layout(set = 1, binding = 0) uniform Uni2
{
	float vars[16];
} uni2;

layout(push_constant) uniform Pc
{
	vec4 vals;
} pc;

void main()
{
	outpos = pos;
}
