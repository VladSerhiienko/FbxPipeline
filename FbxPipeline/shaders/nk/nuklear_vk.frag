#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=0, binding=0) uniform sampler2D samplerFont;

layout(location=0) in vec2 inTexcoords;
layout(location=1) in vec4 inColor;

layout(location=0) out vec4 outColor;

void main(){
   outColor = inColor * texture(samplerFont, inTexcoords.st);
}