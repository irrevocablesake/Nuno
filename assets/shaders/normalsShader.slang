struct PushConstants {
    float dt;
    float elapsedtime;
    uint32_t count;
};

[[vk::push_constant]]
PushConstants push;

[[vk::binding(0, 0)]]
StructuredBuffer<float4> positions;

[[vk::binding(6, 0)]]
StructuredBuffer<uint32_t> triangleCount;

[[vk::binding(7, 0)]]
StructuredBuffer<uint32_t> triangles;

[[vk::binding(8, 0)]]
RWStructuredBuffer< float4 > normals;

[shader("compute")]
[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadID: SV_DispatchThreadID ) {
    uint threadID = dispatchThreadID.x;
    if (threadID >= push.count) return;

    float3 accumulatedNormal = float3(0.0);

    // Range for this vertex
    uint start = triangleCount[threadID];
    uint end = triangleCount[threadID + 1];

    // Every triangle occupies 3 indices
    for (uint i = start; i < end; i += 3) {

        uint i0 = triangles[i + 0];
        uint i1 = triangles[i + 1];
        uint i2 = triangles[i + 2];

        float3 p0 = positions[i0].xyz;
        float3 p1 = positions[i1].xyz;
        float3 p2 = positions[i2].xyz;

        float3 edge1 = p1 - p0;
        float3 edge2 = p2 - p0;

        // Area weighted normal
        float3 faceNormal = cross(edge1, edge2);

        accumulatedNormal += faceNormal;
    }

    accumulatedNormal = normalize(accumulatedNormal);

    normals[threadID] = float4(accumulatedNormal, 0.0);
}