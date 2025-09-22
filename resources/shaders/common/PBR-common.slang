module PBR_common;

import common;

// Computes the Fresnel reflectance term using the Schlick approximation
public float3 FresnelSchlick(float3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0); // clamp is needed just in case if some floating point error would happen
}

// Computes the normal distribution function (NDF) for microfacet surfaces using the GGX (Trowbridge-Reitz) model
public float DistributionGGX(float3 halfway, float3 normal, float roughness)
{
	float a = roughness * roughness; 
	float aSq = a * a;
	float NdotH = max(dot(normal, halfway), 0.0);
	float NdotHSq = NdotH * NdotH;

    float denom = NdotHSq * (aSq - 1.0) + 1.0;
    denom = Pi * denom * denom;

	return aSq / denom;
}

// Computes the geometric attenuation (shadowing-masking) term for a single direction using the Schlick-GGX approximation.
public float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float kDirect = (r * r) / 8.0;

	float denom = NdotV * (1.0 - kDirect) + kDirect;

	return NdotV / denom;
}

// Computes the combined geometric attenuation (shadowing-masking) term for both view and light directions using the Smith method with Schlick-GGX approximation.
public float GeometrySmith(float3 N, float3 V, float3 wi, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, wi), 0.0); // wi - light's direction

	float ggx1 = GeometrySchlickGGX(NdotV, roughness);
	float ggx2 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}