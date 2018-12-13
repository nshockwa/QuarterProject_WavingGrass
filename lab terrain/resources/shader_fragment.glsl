#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;
uniform vec3 campos;
uniform vec3 camoff;

uniform sampler2D tex;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform sampler2D tex6;
uniform sampler2D tex7;
uniform sampler2D tex8;
uniform sampler2D tex9;
uniform sampler2D tex10;

float hash(float n) { return fract(sin(n) * 753.5453123); }
float snoise(vec3 x)
{
	vec3 p = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);

	float n = p.x + p.y * 157.0 + 113.0 * p.z;
	return mix(mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
		mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y),
		mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
			mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
}
//Changing octaves, frequency and presistance results in a total different landscape.
float noise(vec3 position, int octaves, float frequency, float persistence) {
	float total = 0.0;
	float maxAmplitude = 0.0;
	float amplitude = 1.0;
	for (int i = 0; i < octaves; i++) {
		total += snoise(position * frequency) * amplitude;
		frequency *= 2.0;
		maxAmplitude += amplitude;
		amplitude *= persistence;
	}
	return total / maxAmplitude;
}



void main()
{
vec3 n = normalize(vertex_normal);
vec3 lp = vec3(10, -20, -100);
vec3 ld = normalize(vertex_pos - lp);
float diffuse = dot(n, ld);

vec4 texColor;

float noiseval = noise(vertex_pos.xzy, 11, 0.03, 0.6);
if (noiseval < 0.1)
{
	texColor = texture(tex, vertex_tex);
}
else if (noiseval < 0.2)
{
	texColor = texture(tex2, vertex_tex);
}
else if (noiseval < 0.3)
{
	texColor = texture(tex3, vertex_tex);

}
else if (noiseval < 0.4)
{
	texColor = texture(tex4, vertex_tex);
}
else if (noiseval < 0.5) {
	texColor = texture(tex5, vertex_tex);
}
else if (noiseval < 0.6)
{
	texColor = texture(tex6, vertex_tex);
}
else if (noiseval < 0.7)
{
	texColor = texture(tex7, vertex_tex);
}
else if (noiseval < 0.8)
{
	texColor = texture(tex8, vertex_tex);
}
else if (noiseval < 0.9)
{
	texColor = texture(tex9, vertex_tex);
}
else if (noiseval < 1.0)
{
	texColor = texture(tex10, vertex_tex);
}

if (texColor.a < 0.1);
{
	//discard;
}
color = texColor;

return;



color.rgb *= diffuse*0.7;

vec3 cd = normalize(vertex_pos - campos);
vec3 h = normalize(cd+ld);
float spec = dot(n,h);
spec = clamp(spec,0,1);
spec = pow(spec,20);
color.rgb += vec3(1,1,1)*spec*3;
color.a=1;


//float len = length(vertex_pos.xz+campos.xz);
//len-=41;
//len/=8.;
//len=clamp(len,0,1);
//color.a=1-len;
}
