$input v_texcoord0
 
#include "common.sh"

SAMPLER2D(s_colorTexture, 0);
uniform vec4 u_blurParams;

#define u_resolution u_blurParams.xy
#define u_direction  u_blurParams.zw

vec4 blur13(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
  vec4 color = vec4_splat(0.0);
  vec2 off1 = vec2_splat(1.411764705882353) * direction;
  vec2 off2 = vec2_splat(3.2941176470588234) * direction;
  vec2 off3 = vec2_splat(5.176470588235294) * direction;
  color += texture2D(image, uv) * 0.1964825501511404;
  color += texture2D(image, uv + (off1 / resolution)) * 0.2969069646728344;
  color += texture2D(image, uv - (off1 / resolution)) * 0.2969069646728344;
  color += texture2D(image, uv + (off2 / resolution)) * 0.09447039785044732;
  color += texture2D(image, uv - (off2 / resolution)) * 0.09447039785044732;
  color += texture2D(image, uv + (off3 / resolution)) * 0.010381362401148057;
  color += texture2D(image, uv - (off3 / resolution)) * 0.010381362401148057;
  return color;
}

void main()
{
	gl_FragColor = blur13(s_colorTexture, v_texcoord0, u_resolution, u_direction);
}
