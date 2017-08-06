#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( std140, binding = 0 ) uniform FrameUniformBuffer {
    mat4 worldMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 color;
    vec4 positionOffset;
    vec4 positionScale;
} frameInfo;

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec4 inTangent;
layout( location = 3 ) in vec2 inTexcoords;
layout( location = 0 ) out vec4 outColor;

void main( ) {
    outColor    = frameInfo.color;
    gl_Position = frameInfo.projectionMatrix * frameInfo.viewMatrix * frameInfo.worldMatrix *
                  vec4( inPosition.zyx * frameInfo.positionScale.xyz + frameInfo.positionOffset.xyz, 1.0 );
}