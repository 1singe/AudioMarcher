#version 430 core

#include "constants.glsl"
#include "sound_processing.glsl"
#include "distance_functions.glsl"

#define TEST 0

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

out vec4 fragColor;

void main()
{
	fragColor = displayFrequencies(vuv, sound_spectro_left, sound_spectro_right, BUFFER_SIZE);
}