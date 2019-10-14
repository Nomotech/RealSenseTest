#version 430 core

uniform int time;
uniform vec2 resolution;
uniform sampler2D image;
uniform sampler2D label;
uniform int type;
uniform mat4 MVP;

struct Intrin {
  float ppx;
  float ppy;
  float fx;
  float fy;
  float coeffs[5];
};

out vec4 fragColor;
float PI = 3.1415926535;
float elevation = 65.6 * PI / 180.0;
float depression = 91.2 * PI / 180.0;


layout (location = 2) in vec3 position;
uniform sampler2D ;

vec3 coodinate (vec2 pixel, Intrin intrin) {
  float x = (pixel.x - intrin.ppx) / intrin.fx;
  float y = (pixel.y - intrin.ppy) / intrin.fy;
  float r2 = x * x + y * y;
  float f = 1 + intrin.coeffs[0] * r2 + intrin.coeffs[1] * r2 * r2 + intrin.coeffs[4] * r2 * r2 * r2;
  float ux = x * f + 2 * intrin.coeffs[2] * x * y + intrin.coeffs[3] * (r2 + 2 * x * x);
  float uy = y * f + 2 * intrin.coeffs[3] * x * y + intrin.coeffs[2] * (r2 + 2 * y * y);
  return vec3(ux, uy, -1.0);
}

void main(void) {
  Intrin intrin;
  intrin.ppx = 422.563812255859;
  intrin.ppy = 230.812744140625;
  intrin.fx = 423.784362792969;
  intrin.fy = 423.784362792969;
  intrin.coeffs = float[](0, 0, 0, 0, 0);
  vec2 uv = position.xy / resolution.xy;
	uv.y = 1.0 - uv.y;
  float data = texture(image, uv).x;
  float labe = texture(label, uv).x;
	float depth = 1000000.0;
  if (data > 0.0) {
    depth = -(data - 0.05) * 10000.0;
  }

  // vec2 p = (position.xy - resolution.xy / 2.0);
  // float phi = tan(2.0 * p.x * atan(depression / 2.0) / resolution.x);
  // float theta = PI / 2.0 + tan(2.0 * p.y * atan(elevation / 2.0) / resolution.y);

  // vec3 pos = vec3(
  //   sin(theta) * cos(phi),
  //   sin(theta) * sin(phi),
  //   cos(theta)
  // );

  vec3 pos = coodinate(position.xy, intrin) * data * 1000.0;

  //gl_Position = MVP * vec4(position.xy - resolution.xy / 2, 1.0, 1);
  // gl_Position = MVP * vec4(position.xy - resolution.xy / 2, depth, 1);
  gl_Position = MVP * vec4(pos + vec3(0.0, 0.0, 50.0), 1.0);
  fragColor = vec4(data, data, data, labe);
}