struct VSInput {
    uint vertexID : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput vertexShader( VSInput input ) {
    VSOutput output;

    output.position = positions[input.vertexID];
   
    return output;
}

[shader("fragment")]
float4 fragmentShader(VSOutput input) : SV_Target {
    return float4( 1.0, 0.0, 0.0, 1.0 );
}

struct Constraints {
    uint32_t a;
    uint32_t b;

    float restLength;
    float padding;
};

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