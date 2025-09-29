import random;
import common;

public struct UniformSampler
{
    public float3 UniformOnHemisphere(inout float seed)
    {
        float3 random = Hash3(seed);

        float z = random.x;
        float r = sqrt(max(0.0f, 1.0f - z * z));
        float phi = 2 * Pi * random.y;

        return float3(r * cos(phi), r * sin(phi), z);
    }
};