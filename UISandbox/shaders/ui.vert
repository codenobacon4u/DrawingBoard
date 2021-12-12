#version 450

layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location=0) out vec4 fragColor;
layout(location=1) out vec2 fragUV;

void main()
{
    gl_Position = vec4(pc.uScale * inPos + pc.uTranslate, 0.0, 1.0);
    fragColor = inColor;
    fragUV  = inUV;
}