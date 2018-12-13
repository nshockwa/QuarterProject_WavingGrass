#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
layout(location = 3) in vec3 instancePosOffset;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 camoff;
uniform float timeStamp;
uniform vec3 wind;

out vec3 vertex_pos;
out vec3 vertex_normal;
out vec2 vertex_tex;
out int instanceId;

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

//vec3 calcTranslation(vec3 wind_dir, vec3 wind_scale, float t) {
//	return wind_dir * wind_scale * (sin(t) + 0.5);
//}
mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}
void main()
{
	vertex_normal = vec4(M * vec4(vertNor,0.0)).xyz;

vec4 tpos =  M * vec4(vertPos + instancePosOffset, 1.0);



tpos.z -=camoff.z;
tpos.x -=camoff.x;

float scale =  noise(tpos.xzy, 11, 0.03, 0.6);
scale*=60;
	vec3 pos = vertPos;
	pos*=scale*0.15;
	
	
	mat4 R=rotationMatrix(vec3(0,1,0),scale);




tpos =  M *R* vec4(pos, 1.0)+ vec4(instancePosOffset,1);
tpos.w=1;
tpos.z -=camoff.z;
tpos.x -=camoff.x;

//I.e. (vertex shader):
float height = noise(tpos.xzy, 11, 0.03, 0.6);

float baseheight = noise(tpos.xzy, 4, 0.004, 0.3);
baseheight = pow(baseheight, 5)*3;
height = baseheight*height;
height*=60;



	tpos.y += height;
	vertex_pos = tpos.xyz;
	if (vertTex.y < 0.1) {

		//vec3 vVertexTranslation = calcTranslation(wind, wind, time);
		//vec3 tpos2 = vertex_pos + vVertexTranslation;
		vec3 tpos2 = vertex_pos + vec3(noise(tpos.xzy, 3, 0.024, 0.5), noise(tpos.xzy, 7, 0.006, 0.1), noise(tpos.xzy, 2, 0.104, 0.5)) * (sin(timeStamp *2) + scale/60);
		vertex_pos = tpos2;
	}
	vertex_tex = vertTex;
	gl_Position = P * V * vec4(vertex_pos,1.0);
	instanceId = gl_InstanceID;
}
