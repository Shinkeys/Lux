//
// Hash functions by Nimitz:
// https://www.shadertoy.com/view/Xt3cDn
//

import common;

public uint BaseHash(uint2 p) {
    p = 1103515245U * ((p >> 1U) ^ (p.yx));
    uint h32 = 1103515245U * ((p.x) ^ (p.y >> 3U));
    return h32 ^ (h32 >> 16);
}

public float3 Hash3(inout float seed)
{
    uint n = BaseHash(asuint(float2(seed += .1, seed += .1)));
    uint3 rz = uint3(n, n * 16807U, n * 48271U);
    uint x = rz.x & uint(0x7fffffffU);
    uint y = rz.y & uint(0x7fffffffU);
    uint z = rz.z & uint(0x7fffffffU);
    return float3(uint3(x, y, z)) / float(0x7fffffff);
}

public float3 RandomOnHemisphere(inout float seed, float3 normal)
{
    float3 random = Hash3(seed);

    random = random * float3(Two_Pi, 1.0f, 1.0f);
    float theta = random.x; // polar
    float sinP = sqrt(1.0f - random.y * random.y); // azimuth
    float r = pow(random.z, 0.3333333334); // cube root  

    float3 res = r * float3(cos(theta) * sinP, random.y, sin(theta) * sinP);
    if (dot(res, normal) < 0.0f)
        return -res;
    return res;
}