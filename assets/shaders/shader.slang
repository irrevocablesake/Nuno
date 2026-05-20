struct VSInput {
    uint vertexID : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

[[vk::binding(1, 0)]]
StructuredBuffer<float4> predictedPositionsREAD;

[shader("vertex")]
VSOutput vertexShader( VSInput input ) {
    VSOutput output;

    output.position = predictedPositionsREAD[input.vertexID];
   
    return output;
}

[shader("fragment")]
float4 fragmentShader(VSOutput input) : SV_Target {
    return float4( 1.0, 0.0, 0.0, 1.0 );
}

static const float dt = 0.16;
static const float3 gravity = float3( 0.0, 9.81, 0.0 );

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

[shader("compute")]
[numthreads(32, 1, 1)]
void predictionShader(uint3 dispatchThreadID: SV_DispatchThreadID) {
    uint threadID = dispatchThreadID.x;
    if (threadID > 441 ) return;

    float w = mass[threadID];

    if (w > 0.0) {
        velocities[threadID].xyz += gravity * dt;
        predictedPositions[threadID] = positions[threadID] + velocities[threadID] * dt;
    }
    else {
        predictedPositions[threadID] = positions[threadID];
        velocities[threadID] = float4(0.0);
    }
}