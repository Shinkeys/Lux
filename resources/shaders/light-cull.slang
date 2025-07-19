import common.camera;
import common.common;
import common.lights;

[[vk::push_constant]]
cbuffer PushConstants
{
    PointLight *lightsPtr;
    int *lightIndicesPtr;
    ViewData *viewDataPtr;

    uint lightsCount;
    uint maxLightsPerCluster;
    uint tileSize;
};

// Resource to visualize:
// Equations of planes
// https://web.ma.utexas.edu/   users/m408m/Display12-5-3.shtml
float4 CreatePlaneForFrustum(float3 eyePos, float3 tileP1, float3 tileP2)
{
    float3 p0 = tileP1 - eyePos;
    float3 p1 = tileP2 - eyePos;

    float4 planeNormal;
    planeNormal.xyz = normalize(cross(p0, p1));
    planeNormal.w = dot(planeNormal.xyz, eyePos);

    return planeNormal;
}

float4 ClipToView(float4 data)
{
    // Convert clip to view
    float4 dataView = mul(viewDataPtr.inverseProjection, data);
    // Perspective division
    dataView = dataView / dataView.w;

    return dataView;
}

float4 ScreenToView(float4 data)
{
    float2 NDC;

    int2 screenExtent = viewDataPtr.extent;

    NDC.x = (2.0 * data.x) / screenExtent.x - 1.0;
    NDC.y = (2.0 * data.y) / screenExtent.y - 1.0;

    float4 clip = float4(float2(NDC), data.z, data.w);

    return ClipToView(clip);
}

static const int LOCAL_LIGHT_INDICES_MAX_SIZE = 128;
static const int BLOCK_SIZE = 16;

static groupshared int minDepth;
static groupshared int maxDepth;
static groupshared uint localLightCount;
static groupshared float4 frustumPlanes[6];
static groupshared int localLightIndices[LOCAL_LIGHT_INDICES_MAX_SIZE];

void AppendLightOpaque(uint lightIndex)
{
    uint index;
    InterlockedAdd(localLightCount, 1, index);
    if (index >= maxLightsPerCluster) return;

    localLightIndices[index] = int(lightIndex);
}

[vk::binding(1, 0)]
public RWTexture2D<uint2> lightsGrid;
[vk::binding(2, 0)]
public Sampler2D depthTexture;

uint3 GetWorkgroupCount()
{
    __target_switch
    {
    case glsl: __intrinsic_asm "gl_NumWorkGroups";
    case spirv:
        return spirv_asm {
                result:$$uint3 = OpLoad builtin(NumWorkgroups:uint3);
           };
    }
}


[shader("compute")]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void ComputeMain(uint3 threadId: SV_DispatchThreadID, uint3 workGroupID: SV_GroupID, int groupIndex: SV_GroupIndex)
{
    // Sampling depth texture   
    int2 texCoord = int2(threadId.xy);
    float depthFloat = depthTexture.Load(int3(texCoord, 0)).r; // TO VERIFY
    depthFloat = depthFloat * 2.0 - 1.0;
    int depthInt = asuint(depthFloat);


    if (groupIndex == 0)
    {
        minDepth = 0x7FFFFFFF;
        maxDepth = 0;
        localLightCount = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedMin(minDepth, depthInt);
    InterlockedMax(maxDepth, depthInt);

    GroupMemoryBarrierWithGroupSync();

    float minDepthFloat = asfloat(minDepth);
    float maxDepthFloat = asfloat(maxDepth);

    if (groupIndex == 0)
    {
        float3 eyePos = float3(0.0, 0.0, 0.0); // eye pos in view space is origin

        float4 pointsInScreenNear[4];
        pointsInScreenNear[0] = float4(workGroupID.xy * tileSize, minDepthFloat, 1.0);
        pointsInScreenNear[1] = float4(float2(workGroupID.x + 1, workGroupID.y) * tileSize, minDepthFloat, 1.0);
        pointsInScreenNear[2] = float4(float2(workGroupID.x, workGroupID.y + 1) * tileSize, minDepthFloat, 1.0);
        pointsInScreenNear[3] = float4(float2(workGroupID.x + 1, workGroupID.y + 1) * tileSize, minDepthFloat, 1.0);

        float3 pointsInViewNear[4];
        for (int i = 0; i < 4; ++i)
        {
            pointsInViewNear[i] = ScreenToView(pointsInScreenNear[i]).xyz;
        }

        // left
        frustumPlanes[0] = CreatePlaneForFrustum(eyePos, pointsInViewNear[2], pointsInViewNear[0]);
        // right
        frustumPlanes[1] = CreatePlaneForFrustum(eyePos, pointsInViewNear[1], pointsInViewNear[3]);
        // bottom
        frustumPlanes[2] = CreatePlaneForFrustum(eyePos, pointsInViewNear[0], pointsInViewNear[1]);
        // top
        frustumPlanes[3] = CreatePlaneForFrustum(eyePos, pointsInViewNear[3], pointsInViewNear[2]);

        float4 pointsInScreenFar[2];
        pointsInScreenFar[0] = float4(float2(workGroupID.x, workGroupID.y + 1) * tileSize, maxDepthFloat, 1.0);
        pointsInScreenFar[1] = float4(float2(workGroupID.x + 1, workGroupID.y + 1) * tileSize, maxDepthFloat, 1.0);

        float3 pointsInViewFar[2];
        pointsInViewFar[0] = ScreenToView(pointsInScreenFar[0]).xyz;
        pointsInViewFar[1] = ScreenToView(pointsInScreenFar[1]).xyz;

        // far
        frustumPlanes[4] = CreatePlaneForFrustum(eyePos, pointsInViewFar[0], pointsInViewFar[1]);

        // near
        frustumPlanes[5] = CreatePlaneForFrustum(eyePos, pointsInViewNear[3], pointsInViewNear[2]);
    }

    GroupMemoryBarrierWithGroupSync();


    int lightStep = int(BLOCK_SIZE * BLOCK_SIZE);
    for (uint i = groupIndex; i < lightsCount; i += lightStep)
    {
        float3 pos = lightsPtr[i].position;
        float radius = lightsPtr[i].radius;

        float4 lightPosView = mul(viewDataPtr.view, float4(pos, 1.0));

        float nearPlane = 0.0;
        //
        if (lightPosView.z - radius > nearPlane)
            continue;

        float dist = -1.0;

        for (uint j = 0; j < 6; ++j)
        {
            dist = dot(frustumPlanes[j].xyz, lightPosView.xyz) - frustumPlanes[j].w;

            if (dist < -radius) // light outside
                break;
        }

        // && !(dot(minPlane.xyz, lightPosView.xyz) - minPlane.w < -radius)
        if (dist >= -radius)
        {
            AppendLightOpaque(i);
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (groupIndex == 0)
    {
        uint3 workgroupSize = GetWorkgroupCount();

        uint offset = (workGroupID.y * workgroupSize.x + workGroupID.x) * maxLightsPerCluster;
        for (uint i = 0; i < localLightCount; ++i)
        {
            lightIndicesPtr[offset + i] = localLightIndices[i];
        }

        uint startIndex = offset;

        int2 coords = int2(workGroupID.x, workgroupSize.y - workGroupID.y);
        lightsGrid.Store(uint2(coords), uint2(startIndex, localLightCount));
    }
}