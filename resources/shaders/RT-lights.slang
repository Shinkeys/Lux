import common.camera;
import common.RT_common;
import common.mesh_common;
import common.lights;

// If hit light just return color white
[shader("closesthit")]
void LightsClosestMain(inout RayPayload payload)
{
    payload.color = float4(1.0f, 0.7f, 0.85f, 1.0f);
    payload.dot = 1.0f;
    payload.reflector = false;
}