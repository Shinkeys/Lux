module lights;

public float Square(float x)
{
    return x * x;
}

public float AttenuatePointLight(float3 lightPos, float3 fragPos, float intensity, float radius)
{
    const float distance = length(lightPos - fragPos);
    const float decaySpeed = 1.0;
    const float maxIntensity = intensity; // basically represents 'start' point of the light brightness
    const float s = distance / radius;
    const float sqrS = Square(s);
    // check if distance < radius, otherwise would get wrong lightness
    // values at larger distances
    if (s >= 1.0)
        return 0.0;

    return maxIntensity * Square(1 - sqrS) / (1 + decaySpeed * sqrS);
}

public struct PointLight
{
    public float3 position;
    public float3 color;
    public float intensity;
    public float radius;
};

