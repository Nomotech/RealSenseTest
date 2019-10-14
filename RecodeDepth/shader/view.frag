#version 430 core

uniform int time;
uniform vec2 resolution;
uniform sampler2D image;
uniform int type;

out vec4 FragColor;

const float PI = 3.141592;
vec3 HSVtoRGB(vec3 hsv) {
	vec3 col;
	float hue = mod(hsv.r, 360.0);
	float s = max(0, min(1, hsv.g));
	float v = max(0, min(1, hsv.b));
	if(s > 0.0) {
		int h = int(floor(hue / 60.0));
		float f = hue / 60.0 - float(h);
		float p = v * (1.0 - s);
		float q = v * (1.0 - f * s);
		float r = v * (1.0 - (1.0 - f) * s);

		if(h == 0) col = vec3(v, r, p);
		else if(h == 1) col = vec3(q, v, p);
		else if(h == 2) col = vec3(p, v, r);
		else if(h == 3) col = vec3(p, q, v);
		else if(h == 4) col = vec3(r, p, v);
		else col = vec3(v, p, q);
	}else{
		col = vec3(v);
	}
	return col;
}

vec4 smoothing(sampler2D tex) {
	vec2 pos = gl_FragCoord.xy / resolution.xy;
	pos.y = 1.0 - pos.y;

    vec4 p00 = textureOffset(tex, pos, ivec2(-1, -1));
    vec4 p01 = textureOffset(tex, pos, ivec2(0, -1));
    vec4 p02 = textureOffset(tex, pos, ivec2(1, -1));
    vec4 p10 = textureOffset(tex, pos, ivec2(-1, 0));
    vec4 p11 = textureOffset(tex, pos, ivec2(0, 0));
    vec4 p12 = textureOffset(tex, pos, ivec2(1, 0));
    vec4 p20 = textureOffset(tex, pos, ivec2(-1, 1));
    vec4 p21 = textureOffset(tex, pos, ivec2(0, 1));
    vec4 p22 = textureOffset(tex, pos, ivec2(1, 1));

	return (p00 + p01 + p02 + p10 + p11 + p12 + p20 + p21 + p22) / 9.0;
}

void main( void ) {
	vec2 pos = gl_FragCoord.xy / resolution.xy;
	pos.y = 1.0 - pos.y;
	vec3 color = vec3(0.0);
	float depth = texture(image, pos).x;
	color = vec3(depth);
	if (type == 1) {
		color.r += 0.5;
	}
	FragColor = vec4(color, 1.0);
}