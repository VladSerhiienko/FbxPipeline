$input a_position, a_normal, a_tangent, a_texcoord0
$output v_view, v_normal, v_logz

#include "common.sh"
#include "uniforms.sh"

void main()
{
	// Calculate position
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );

	// Calculate log depth
	// http://outerra.blogspot.com/2013/07/logarithmic-depth-buffer-optimizations.html
	float Fcoef = 2.0 / log2(u_camFar + 1.0);
    gl_Position.z = log2(max(1e-6, 1.0 + gl_Position.w)) * Fcoef - 1.0;
	v_logz = 1.0 + gl_Position.w;

	// View direction
	v_view = u_camPos - mul(u_model[0], vec4(a_position, 1.0)).xyz;

	// Normal in world space
	vec3 normal = a_normal;
	v_normal = mul(u_model[0], vec4(normal, 0.0) ).xyz;
}
