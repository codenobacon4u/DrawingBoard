#version 450

layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
	gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
	fragColor = aColor;
	fragTexCoord = aUV;
}
