#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 i_uv;

layout(location = 0) out vec4 o_color;

layout(push_constant) uniform Pc 
{
  vec4 color;
	vec2 twice_inv_size;
	uint texture_index;
} pc;

layout(set = 0, binding = 0) uniform sampler u_immutable_samplers;
layout(set = 0, binding = 1) uniform texture2D u_textures[];

void main()
{
	o_color = pc.color;
  o_color.a *= texture(sampler2D(u_textures[pc.texture_index], u_immutable_samplers), i_uv).r;
}
