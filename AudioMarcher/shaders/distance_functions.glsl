float sdPlane(vec3 p)
{
	return p.y;
}

float sdSphere(vec3 p, float s)
{
	return length(p) - s;
}

float sdBox(vec3 p, vec3 b)
{
	vec3 q = abs(p) - b;
	return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdRoundBox(vec3 p, vec3 b, float r)
{
	vec3 q = abs(p) - b + r;
	return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - r;
}

float sdCylinder(vec3 p, vec3 c)
{
	return length(p.xz - c.xy) - c.z;
}

float sdCone_finite(vec3 p, vec2 c, float h)
{
	// c is the sin/cos of the angle, h is height
	// Alternatively pass q instead of (c,h),
	// which is the point at the base in 2D
	vec2 q = h * vec2(c.x / c.y, -1.0);

	vec2 w = vec2(length(p.xz), p.y);
	vec2 a = w - q * clamp(dot(w, q) / dot(q, q), 0.0, 1.0);
	vec2 b = w - q * vec2(clamp(w.x / q.x, 0.0, 1.0), 1.0);
	float k = sign(q.y);
	float d = min(dot(a, a), dot(b, b));
	float s = max(k * (w.x * q.y - w.y * q.x), k * (w.y - q.y));
	return sqrt(d) * sign(s);
}

float sdCone(vec3 p, vec2 c)
{
	// c is the sin/cos of the angle
	vec2 q = vec2(length(p.xz), -p.y);
	float d = length(q - c * max(dot(q, c), 0.0));
	return d * ((q.x * c.y - q.y * c.x < 0.0) ? -1.0 : 1.0);
}

float sdOctahedron(vec3 p, float s)
{
	p = abs(p);
	float m = p.x + p.y + p.z - s;
	vec3 q;
	if (3.0 * p.x < m) q = p.xyz;
	else if (3.0 * p.y < m) q = p.yzx;
	else if (3.0 * p.z < m) q = p.zxy;
	else return m * 0.57735027;

	float k = clamp(0.5 * (q.z - q.y + s), 0.0, s);
	return length(vec3(q.x, q.y - s + k, q.z - k));
}

float sdVesicaSegment(in vec3 p, in vec3 a, in vec3 b, in float w)
{
	vec3 c = (a + b) * 0.5;
	float l = length(b - a);
	vec3 v = (b - a) / l;
	float y = dot(p - c, v);
	vec2 q = vec2(length(p - c - y * v), abs(y));

	float r = 0.5 * l;
	float d = 0.5 * (r * r - w * w) / w;
	vec3 h = (r * q.x < d * (q.y - r)) ? vec3(0.0, r, 0.0) : vec3(-d, 0.0, d + w);

	return length(q - h.xy) - h.z;
}


float sdCutSphere(vec3 p, float r, float h)
{
	// sampling independent computations (only depend on shape)
	float f = r * r - h * h;
	float w = sqrt(f);

	// sampling dependant computations
	vec2 q = vec2(length(p.xz), p.y);
	float s = max((h - r) * q.x * q.x + w * w * (h + r - 2.0 * q.y), h * q.x - w * q.y);
	return (s < 0.0) ? length(q) - r : (q.x < w) ? h - q.y : length(q - vec2(w, h));
}
//
//
//vec3 opTx( in vec3 p, in transform t, in sdf3d primitive )
//{
//	return primitive( invert(t)*p );
//}
//
//
//float opRepetition( in vec3 p, in vec3 s, in sdf3d primitive )
//{
//	vec3 q = p - s*round(p/s);
//	return primitive( q );
//}
//
//
//vec3 opLimitedRepetition( in vec3 p, in float s, in vec3 l, in sdf3d primitive )
//{
//	vec3 q = p - s*clamp(round(p/s),-l,l);
//	return primitive( q );
//}