static const float dt = 0.16;
static const float3 gravity = float3( 1.0, 9.81, 2.0 );

struct Constraints {
    uint32_t a;
    uint32_t b;

    float restLength;
};

struct PushConstants {
    float dt;
    float elapsedTime;
    uint32_t count;

    float padding;

    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float4x4 modelMatrix;
};

[[vk::push_constant]]
PushConstants push;

[[vk::binding(0, 0)]]
StructuredBuffer<float4> positions;

[[vk::binding(1, 0)]]
RWStructuredBuffer<float4> predictedPositions;

[[vk::binding(2, 0)]]
RWStructuredBuffer<float4> velocities;

[[vk::binding(3, 0)]]
StructuredBuffer< float > mass;

[[vk::binding(4, 0)]]
StructuredBuffer<float> lambda;

[[vk::binding(5, 0)]]
StructuredBuffer<Constraints> constraints;

[shader("compute")]
[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadID: SV_DispatchThreadID) {
    uint threadID = dispatchThreadID.x;
    if (threadID >= push.count ) return;

    float w = mass[threadID];

    //Note: For now, we just mess around with gravity, but we need to add external forces later

    if (w > 0.0) {
        velocities[threadID].xyz += gravity * push.dt;
        predictedPositions[threadID] = positions[threadID] + velocities[threadID] * push.dt;
    }
    else {
        predictedPositions[threadID] = positions[threadID];
        velocities[threadID] = float4(0.0);
    }
}
