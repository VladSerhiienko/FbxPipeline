#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants { vec4 offsetScale; } pushConstants;

layout(location=0) in vec2 inPosition;
layout(location=1) in vec2 inTexcoords;
layout(location=2) in vec4 inColor;

layout(location=0) out vec2 outTexcoords;
layout(location=1) out vec4 outColor;

void main() {
    outTexcoords = inTexcoords;
    outColor = inColor;
    gl_Position = vec4(inPosition.xy * pushConstants.offsetScale.zw + pushConstants.offsetScale.xy, 0, 1);
}