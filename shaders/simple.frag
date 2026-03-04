
#version 450

layout(location=0) in vec3 pos;

layout(location=0) out vec3 color;

layout(binding=0) uniform Uni
{
	float vars[16];
} uni;

layout(binding=1) uniform Uni1
{
	float vars[16];
} uni1;

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
	color = pos;
}
