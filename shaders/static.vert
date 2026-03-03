#version 450

layout(location=0) out vec3 color;

vec2 positions[4] = {
	{0.5f, 0.5f},
	{-0.5f, 0.5f},
	{0.5f, -0.5f},
	{-0.5f, -0.5f}
};

vec3 colors[4] = {
	{1.0f, 1.0f, 1.0f},
	{1.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 1.0f},
};

void main()
{
	color = colors[gl_VertexIndex];
	gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
}
