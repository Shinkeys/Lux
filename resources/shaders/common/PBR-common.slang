module PBR_common;

import common;

public float3 FresnelSchlick(float3 F0, float cosTheta) // angle between halfway and viewdir
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0); // clamp is needed just in case if some floating point error would happen
}

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

public float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float kDirect = (r * r) / 8.0;

	float denom = NdotV * (1.0 - kDirect) + kDirect;

	return NdotV / denom;
}

public float GeometrySmith(float3 N, float3 V, float3 wi, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, wi), 0.0); // wi - light's direction

	float ggx1 = GeometrySchlickGGX(NdotV, roughness);
	float ggx2 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}