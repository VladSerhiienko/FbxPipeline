$input v_texcoord0
 
#include "common.sh"

SAMPLER2D(s_colorTexture, 0);
uniform vec4 u_blurParams;

#define u_resolution u_blurParams.xy
#define u_direction  u_blurParams.zw

vec4 blur5(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
  vec4 color = vec4_splat(0.0);
  vec2 off1 = vec2_splat(1.3333333333333333) * direction;
  color += texture2D(image, uv) * 0.29411764705882354;
  color += texture2D(image, uv + (off1 / resolution)) * 0.35294117647058826;
  color += texture2D(image, uv - (off1 / resolution)) * 0.35294117647058826;
  return color; 
}

void main()
{
	gl_FragColor = toFilmic(blur5(s_colorTexture, v_texcoord0, u_resolution, u_direction));
}
