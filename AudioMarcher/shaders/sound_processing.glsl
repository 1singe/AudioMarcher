float isophonic(float x) {
	return sqrt(x) + 0.1f;
}

vec4 displayFrequencies(in vec2 vuv, float[BUFFER_SIZE] bufR, float[BUFFER_SIZE] bufL, int size)
{
	vec4 cl = vec4(1.0, 0.0, 0.0, 1);
	vec4 cr = vec4(0.0, 0.0, 1.0, 1);

	float indexf = vuv.x * float(size);
	int index = int(floor(indexf));
	float ampl = abs(float(bufL[index]));
	float ampr = abs(float(bufR[index]));

	float displayL = vuv.y < ampl / 50.f ? 1.0f : 0.0f;
	float displayR = vuv.y < ampr / 50.f ? 1.0f : 0.0f;

	return cl.xyzw * displayL + cr.xyzw * displayR;
}

int remapFrequencyToIndex(float freq) {
	return int(floor(((freq - 20.0f) / 19980.0f) * BUFFER_SIZE));
}

float getAverageFreqBand(float[BUFFER_SIZE] buf, float freqMin, float freqMax) {
	freqMin = max(20, freqMin);
	freqMax = min(20000, freqMax);
	int minI = remapFrequencyToIndex(freqMin);
	int maxI = remapFrequencyToIndex(freqMax);
	float nSamples = maxI - minI;
	float sum = 0.0f;
	for (int i = minI; i < maxI; i++) {
		sum += buf[i];
	}
	return sum / nSamples;
}