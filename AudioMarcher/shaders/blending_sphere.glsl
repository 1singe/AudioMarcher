#version 430

#include "constants.glsl"
#include "sound_processing.glsl"

// The MIT License
// Copyright © 2024 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// More info: https://iquilezles.org/articles/smin/
in vec2 vuv;

uniform vec2 resolution;
uniform float time;
uniform float sound_spectro_left[BUFFER_SIZE];
uniform float sound_spectro_right[BUFFER_SIZE];

uniform vec3 subF;
uniform vec3 lowF;
uniform vec3 midF;
uniform vec3 highF;

out vec4 fragColor;


// Cubic Polynomial Smooth-minimum
vec2 smin(float a, float b, float k)
{
	k *= 6.0;
	float h = max(k - abs(a - b), 0.0) / k;
	float m = h * h * h * 0.5;
	float s = m * k * (1.0 / 3.0);
	return (a < b) ? vec2(a - s, m) : vec2(b - s, 1.0 - m);
}

float hash1(float n)
{
	return fract(sin(n) * 43758.5453123);
}

vec3 forwardSF(float i, float n)
{
	const float PI = 3.1415926535897932384626433832795;
	const float PHI = 1.6180339887498948482045868343656;
	float phi = 2.0 * PI * fract(i / PHI);
	float zi = 1.0 - (2.0 * i + 1.0) / n;
	float zizi = 1.0 - zi * zi;
	float sinTheta = sqrt(zizi);
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, zi);
}

vec2 map(vec3 q)
{
	// plane
	vec2 res = vec2(q.y, 2.0);

	float y = 0.1f + (getAverageFreqBand(sound_spectro_left, 1400.f, 2000.0f)) * 0.3f;
	float width = 0.05f + 0.01f * (getAverageFreqBand(sound_spectro_left, 200.0f, 800.0f));
	// sphere
	float d = length(q - vec3(0.0, 0.1 + 0.05 * y, 0.0)) - width;

	// smooth union    
	return smin(res.x, d, 0.05);
}

//float height = (getAverageFreqBand(sound_spectro_left, 20.0f, 800.0f) * 0.1f) - 1.f;

vec2 intersect(in vec3 ro, in vec3 rd)
{
	const float maxd = 10.0;

	vec2 res = vec2(0.0);
	float t = 0.0;
	for (int i = 0; i < 512; i++)
	{
		vec2 h = map(ro + rd * t);
		if ((h.x < 0.0) || (t > maxd)) break;
		t += h.x;
		res = vec2(t, h.y);
	}

	if (t > maxd) res = vec2(-1.0);
	return res;
}

// https://iquilezles.org/articles/normalsSDF
vec3 calcNormal(in vec3 pos)
{
	vec2 e = vec2(1.0, -1.0) * 0.5773 * 0.005;
	return normalize(e.xyy * map(pos + e.xyy).x +
	e.yyx * map(pos + e.yyx).x +
	e.yxy * map(pos + e.yxy).x +
	e.xxx * map(pos + e.xxx).x);
}

// https://iquilezles.org/articles/nvscene2008/rwwtt.pdf
float calcAO(in vec3 pos, in vec3 nor, float ran)
{
	float ao = 0.0;
	const int num = 32;
	for (int i = 0; i < num; i++)
	{
		vec3 kk;
		vec3 ap = forwardSF(float(i) + ran, float(num));
		ap *= sign(dot(ap, nor)) * hash1(float(i));
		ao += clamp(map(pos + nor * 0.01 + ap * 0.2).x * 20.0, 0.0, 1.0);
	}
	ao /= float(num);

	return clamp(ao, 0.0, 1.0);
}

vec3 render(in vec2 p, vec4 ran)
{
	//-----------------------------------------------------
	// camera
	//-----------------------------------------------------
	vec3 ro = vec3(0.4, 0.15, 0.4);
	vec3 ta = vec3(0.0, 0.05, 0.0);
	// camera matrix
	vec3 ww = normalize(ta - ro);
	vec3 uu = normalize(cross(ww, vec3(0.0, 1.0, 0.0)));
	vec3 vv = normalize(cross(uu, ww));
	// create view ray
	vec3 rd = normalize(p.x * uu + p.y * vv + 1.7 * ww);

	//-----------------------------------------------------
	// render
	//-----------------------------------------------------

	vec3 col = vec3(1.0);

	// raymarch
	vec3 uvw;
	vec2 res = intersect(ro, rd);
	float t = res.x;
	if (t > 0.0)
	{
		vec3 pos = ro + t * rd;
		vec3 nor = calcNormal(pos);
		vec3 ref = reflect(rd, nor);
		float fre = clamp(1.0 + dot(nor, rd), 0.0, 1.0);
		float occ = calcAO(pos, nor, ran.y); occ = occ * occ;

		// blend materials        
		col = mix(vec3(0.0, 0.05, 1.0),
		          vec3(1.0, 0.0, 0.0),
		          res.y);

		col = col * 0.72 + 0.2 * fre * vec3(1.0, 0.8, 0.2);
		vec3 lin = 4.0 * vec3(0.7, 0.8, 1.0) * (0.5 + 0.5 * nor.y) * occ;
		lin += 0.8 * vec3(1.0, 1.0, 1.0) * fre * (0.6 + 0.4 * occ);
		col = col * lin;
		col += 2.0 * vec3(0.8, 0.9, 1.00) * smoothstep(0.0, 0.4, ref.y) * (0.06 + 0.94 * pow(fre, 5.0)) * occ;
		col = mix(col, vec3(1.0), 1.0 - exp2(-0.04 * t * t));

	}

	// gamma and postpro
	col = pow(col, vec3(0.4545));
	col *= 0.9;
	col = clamp(col, 0.0, 1.0);
	col = col * col * (3.0 - 2.0 * col);

	// dithering
	col += (ran.x - 0.5) / 255.0;

	return col;
}

#define AA 2

void main()
{
	vec3 col = vec3(0.0);
	for (int m = 0; m < AA; m++)
	for (int n = 0; n < AA; n++)
	{
		vec2 px = vuv * resolution + vec2(float(m), float(n)) / float(AA);
		vec4 ran;

		vec2 p = (2.0 * px - resolution.xy) / resolution.y;
		col += render(p, ran);
	}
	col /= float(AA * AA);

	//fragColor = displayFrequencies(vuv, sound_spectro_left, sound_spectro_right, BUFFER_SIZE);
	fragColor = vec4(col, 1.0);
}