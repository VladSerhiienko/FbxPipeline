#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( std140, binding = 0 ) uniform FrameUniformBuffer {
    mat4 ProjMatrix;
    mat4 EnvMatrix;
    vec4 Params0;
    vec4 Params1;
} u_frame;

layout( location = 0 ) in vec3 a_position;
layout( location = 1 ) in vec2 a_texcoords;

layout( location = 0 ) out vec3 v_dir;
layout( location = 1 ) out vec4 v_params0;
layout( location = 2 ) out vec4 v_params1;

void main( ) {
    // https://rauwendaal.net/2014/06/14/rendering-a-screen-covering-triangle-in-opengl/
    vec2 texcoords = vec2( ( gl_VertexIndex << 1 ) & 2, gl_VertexIndex & 2 );
    gl_Position = u_frame.ProjMatrix * vec4( texcoords * 2.0f - 1.0f, 0.0f, 1.0f );

    float fov = radians(55.0);
    float height = tan(fov*0.5);
    float aspect = height*(1200.0 / 800.0);
    vec2 tex = (2.0*texcoords-1.0) * vec2(aspect, height);

    v_dir = normalize( (u_frame.EnvMatrix * vec4( tex, 1.0, 0.0 ) ).xyz );
    v_params0 = u_frame.Params0;
    v_params1 = u_frame.Params1;
}
