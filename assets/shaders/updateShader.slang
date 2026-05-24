struct VSInput {
    uint vertexID : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

struct Constraints {
    uint32_t a;
    uint32_t b;

    float restLength;
};

struct PushConstants {
    float dt;
    float elapsedtime;
    uint32_t count;

    float4 projectionMatrix;
    float4 viewMatrix;
    float4 modelMatrix;
};

[[vk::push_constant]]
PushConstants push;

[[vk::binding(0, 0)]]
RWStructuredBuffer<float4> positions;

[[vk::binding(1, 0)]]
StructuredBuffer<float4> predictedPositions;

[[vk::binding(2, 0)]]
RWStructuredBuffer<float4> velocities;

[[vk::binding(3, 0)]]
StructuredBuffer<float> mass;

[[vk::binding(4, 0)]]
StructuredBuffer<float> lambda;

[[vk::binding(5, 0)]]
StructuredBuffer<Constraints> constraints;

[shader("compute")]
[numthreads(32, 1, 1)]
void main(uint3 id: SV_DispatchThreadID) {
    uint i = id.x;
    if (i >= push.count ) return;
    if (mass[i] == 0.0) return; 

    velocities[i].xyz = (predictedPositions[i].xyz - positions[i].xyz) / push.dt;
    positions[i].xyz = predictedPositions[i].xyz;

    velocities[i].xyz *= 0.95;
}