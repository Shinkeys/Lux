//
// Hash functions by Nimitz:
// https://www.shadertoy.com/view/Xt3cDn
//

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

    if (dot(random, normal) <= 0.0f)
        return -random;

    return random;
}