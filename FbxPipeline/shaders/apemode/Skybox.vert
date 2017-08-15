#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( std140, binding = 0 ) uniform FrameUniformBuffer {
    mat4 projMatrix;
    mat4 envMatrix;
    vec4 params0;
    vec4 params1;

#define u_lerpFactor u_frame.params0.y
#define u_unused u_frame.params1.y
#define u_exposure0 u_frame.params0.x
#define u_exposure1 u_frame.params1.x
#define u_textureCubeDimLod0 u_frame.params0.zw
#define u_textureCubeDimLod1 u_frame.params1.zw

} u_frame;

layout( location = 0 ) in vec3 a_position;
layout( location = 1 ) in vec2 a_texcoords;

layout( location = 0 ) out vec3 v_dir;
layout( location = 1 ) out vec4 v_params0;
layout( location = 2 ) out vec4 v_params1;

void main( ) {
    gl_Position =  u_frame.projMatrix * vec4( a_position, 1.0 );
    v_dir = normalize( (u_frame.envMatrix * vec4( a_texcoords, 1.0, 0.0 ) ).xyz );
    v_params0 = u_frame.params0;
    v_params1 = u_frame.params1;
}
