#version 430

#include "constants.glsl"
#include "sound_processing.glsl" 

in vec2 vuv;
uniform float time;
uniform vec2 resolution;
uniform vec2 volume;
uniform float[] sound_buffer_left;
uniform float[] sound_buffer_right;
uniform float sound_spectro_left[BUFFER_SIZE];
uniform float sound_spectro_right[BUFFER_SIZE];

uniform vec3 subL;
uniform vec3 lowL;
uniform vec3 midL;
uniform vec3 highL;
uniform vec3 subR;
uniform vec3 lowR;
uniform vec3 midR;
uniform vec3 highR;

// Edit values

float gTime = 0.;
const float REPEAT = 5.0;

uniform sampler1D leftFreq;
uniform sampler1D rightFreq;

out vec4 fragColor;

// 回転行列
mat2 rot(float a) {
	float c = cos(a), s = sin(a);
	return mat2(c, s, -s, c);
}

float sdOctahedron(vec3 p, float s)
{
	p = abs(p);
	return (p.x + p.y + p.z - s) * 0.57735027;
}

float sdBox(vec3 p, vec3 b)
{
	vec3 q = abs(p) - b;
	return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdEllipsoid(vec3 p, vec3 r)
{
	float k0 = length(p / r);
	float k1 = length(p / (r * r));
	return k0 * (k0 - 1.0) / k1;
}

float box(vec3 pos, float scale) {
	pos *= scale;
	float base = mix(sdEllipsoid(pos, vec3(.4, .4, .1)), sdBox(pos, vec3(.4, .4, .1)), (cos(gTime) + 1.f) / 2.f) / 1.5;
	pos.xy *= 5.;
	pos.y -= 3.5;
	pos.xy *= rot(.75);
	float result = -base;
	return result;
}

float box_set(vec3 pos, float iTime) {
	vec3 pos_origin = pos;
	pos = pos_origin;
	pos .y += sin(gTime * 0.4) * 2.5;
	pos.xy *= rot(.8);
	float box1 = box(pos, 2. - abs(sin(gTime * 0.4)) * 1.5);
	pos = pos_origin;
	pos .y -= sin(gTime * 0.4) * 2.5;
	pos.xy *= rot(.8);
	float box2 = box(pos, 2. - abs(sin(gTime * 0.4)) * 1.5);
	pos = pos_origin;
	pos .x += sin(gTime * 0.4) * 2.5;
	pos.xy *= rot(.8);
	float box3 = box(pos, 2. - abs(sin(gTime * 0.4)) * 1.5);
	pos = pos_origin;
	pos .x -= sin(gTime * 0.4) * 2.5;
	pos.xy *= rot(.8);
	float box4 = box(pos, 2. - abs(sin(gTime * 0.4)) * 1.5);
	pos = pos_origin;
	pos.xy *= rot(.8);
	float box5 = box(pos, .5) * 6.;
	pos = pos_origin;
	float box6 = box(pos, .5) * 6.;
	float result = max(max(max(max(max(box1, box2), box3), box4), box5), box6);
	return result;
}

float map(vec3 pos, float iTime) {
	vec3 pos_origin = pos;
	float box_set1 = box_set(pos, iTime);

	return box_set1;
}


void main() {
	float Time = (time + (midL.x > midL.z ? midL.x : 0.f) * midL.z) * cos(lowL.x) * lowL.y;

	vec2 p = (vuv.xy * resolution * 2. - resolution) / min(resolution.x, resolution.y);
	vec3 ro = vec3(0., -0.2, time * 4.);
	vec3 ray = normalize(vec3(p, 1.2f));
	ray.xy = ray.xy * rot(sin(time * .01 + lowL.x * lowL.y) * 5.);
	ray.yz = ray.yz * rot(sin(time * .05) * .2);
	float t = 0.1;
	vec3 col = vec3(0.);
	float ac = 0.0;


	for (int i = 0; i < 99; i++) {
		vec3 pos = ro + ray * t;
		pos = mod(pos - 2., 4.) - 2.;
		gTime = Time - float(i) * 0.01;

		float d = map(pos, Time);

		d = max(abs(d), 0.01);
		ac += exp(-d * 23.);

		t += d * 0.55;
	}

	col = vec3(ac * (0.018 + highL.x * highL.y)) + vec3(0.3f, -0.3f, -0.3f) * clamp(sqrt(subL.x) * subL.y, 0., 1.);

	col -= vec3(0., 0.02 * sin(Time), 0.1 * sin(Time)) * 0.4f;


	fragColor = vec4(col, 1.0 - t * (0.02 + 0.02 * sin(Time)));
}

/** SHADERDATA
{
	"title": "Octgrams",
	"description": "Lorem ipsum dolor",
	"model": "person"
}
*/