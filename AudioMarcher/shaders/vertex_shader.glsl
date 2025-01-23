#version 430 core

out vec2 vuv;

void main()
{
	vuv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
	gl_Position = vec4(vuv * 2. - 1., 0.0f, 1.0f);
}