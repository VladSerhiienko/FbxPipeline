#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding = 0) uniform FrameUniformBuffer {
    mat4 worldMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 color;
} frameInfo;

layout (location = 0) in vec3 inPosition;
layout (location = 0) out vec4 outColor;

void main() 
{
   outColor = frameInfo.color;
   gl_Position = frameInfo.projectionMatrix * frameInfo.viewMatrix * frameInfo.worldMatrix * vec4(inPosition, 1.0);
}
