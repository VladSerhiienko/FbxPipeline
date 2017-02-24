$input v_texcoord0
 
#include "common.sh"

SAMPLER2D(s_colorTexture, 0);
uniform vec4 u_blurParams;

#define u_resolution u_blurParams.xy
#define u_direction  u_blurParams.zw

vec4 blur9(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
  vec4 color = vec4_splat(0.0);
  vec2 off1 = vec2_splat(1.3846153846) * direction;
  vec2 off2 = vec2_splat(3.2307692308) * direction;
  color += texture2D(image, uv) * 0.2270270270;
  color += texture2D(image, uv + (off1 / resolution)) * 0.3162162162;
  color += texture2D(image, uv - (off1 / resolution)) * 0.3162162162;
  color += texture2D(image, uv + (off2 / resolution)) * 0.0702702703;
  color += texture2D(image, uv - (off2 / resolution)) * 0.0702702703;
  return color;
}

void main()
{
	gl_FragColor = toFilmic(blur9(s_colorTexture, v_texcoord0, u_resolution, u_direction));
}
