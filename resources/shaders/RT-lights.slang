import common.camera;
import common.RT_common;
import common.mesh_common;
import common.lights;

// If hit light just return color white
[shader("closesthit")]
void LightsClosestMain(inout RayPayload payload)
{
    payload.radiance = float4(3.0f, 3.0f,3.0f, 1.0f);
    payload.distance = -1.0f;
    payload.hitLight = true;
}