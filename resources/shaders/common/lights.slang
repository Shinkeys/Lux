module lights;

import common;
import PBR_common;

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

public float3 CalculateLight(PointLight light, float3 albedo, float3 metallicRoughness, float3 normal, float3 cameraPos, float3 fragPos)
{
    float attenuation = AttenuatePointLight(light.position, fragPos, light.intensity, light.radius);

    float3 wi = normalize(light.position - fragPos); // wi is basically a current light direction incoming to the surf.
    float cosTheta = max(dot(normal, wi), 0.0);
    float3 L = light.color * attenuation * light.intensity; // L is basically a radiance of the rendering equation, cosTheta is the angle between
    float3 V = normalize(cameraPos - fragPos); // view vector(WORLD SPACE)
    // incoming ray and the surface normal
    float3 halfway = normalize(V + wi);

    float metallic = metallicRoughness.b;
    float roughness = metallicRoughness.g;

    // Fresnel
    float3 F0 = float3(0.04);
    F0 = lerp(F0, albedo, metallic); // interpolate for non-metallic surfaces

    float NDF = DistributionGGX(halfway, normal, roughness);
    float G   = GeometrySmith(normal, V, wi, roughness);
    float3 F  = FresnelSchlick(F0, max(dot(halfway, V), 0.0));

    float3 DFG = NDF * G * F;
    float cookTorranceDenom = 4 * max(dot(normal, V), 0.0) * max(dot(normal, wi), 0.0) + 0.0001; // prevent zero division
    float3 specular = DFG / cookTorranceDenom;

    float3 kS = F; // Fresnel is just a specular
    float3 kD = float3(1.0) - kS;

    kD *= 1.0 - metallic; // if metallic - no diffuse

    float NdotL = max(dot(normal, wi), 0.0);

    return (kD * albedo / Pi + specular) * L * NdotL;
}


public struct PointLight
{
    public float3 position;
    public float3 color;
    public float intensity;
    public float radius;
};