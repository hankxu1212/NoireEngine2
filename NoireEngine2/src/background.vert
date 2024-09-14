#version 450 //GLSL version 4.5

layout(location = 0) out vec2 position;

void main() {
	vec2 POSITION = vec2(2 * (gl_VertexIndex & 2) - 1, 4 * (gl_VertexIndex & 1) - 1);

	position = POSITION * 0.5 + 0.5;

	gl_Position = vec4(POSITION, 0.0, 1.0);
}