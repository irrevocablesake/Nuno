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

[[vk::binding(0, 0)]]
StructuredBuffer<float4> positions;

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

[shader("vertex")]
VSOutput main( VSInput input ) {
    VSOutput output;

    output.position = positions[input.vertexID];
   
    return output;
}