$input v_texcoord0
 
#include "common.sh"

SAMPLER2D(s_colorTexture, 0);
SAMPLER2D(s_blurredTexture, 1);
SAMPLER2D(s_maskTexture, 2);

void main()
{
	vec2 maskCoords = vec2(0.0, 1.0) +  v_texcoord0 * vec2(1.0, -1.0);
	vec3 color 		= texture2D (s_colorTexture, v_texcoord0).rgb;
	vec3 blurred 	= texture2D (s_blurredTexture, v_texcoord0).rgb;
	vec3 mask 		= texture2D (s_maskTexture, maskCoords).aaa;
	vec3 combined 	= color * (vec3_splat(1.0) - mask) + blurred * mask;
	gl_FragColor 	= vec4(combined, 1.0);
}
