struct VSInput {
    uint vertexID : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

[[vk::binding(0, 0)]]

StructuredBuffer<Particle> particles0;
[shader("vertex")]
VSOutput main( VSInput input ) {
    VSOutput output;

    float2 position[3] = {
        float2( -0.75, -0.75 ),
        float2( 0.5, 0.75 ),
        float2( -0.75, 0.75  ),
    };

    output.position = float4(particles0[input.vertexID].position);
   
    return output;
}

[shader("fragment")]
float4 main(VSOutput input) : SV_Target {
    return float4( 1.0, 0.0, 0.0, 1.0 );
}

struct Particle {
    float4 position;
};

[[vk::binding(0, 0)]]
RWStructuredBuffer<Particle> particles;

[shader("compute")]
[numthreads(1,1,1)]
void main(uint3 dispatcheThreadID: SV_DispatchThreadID) {
    uint id = dispatcheThreadID.x;

    particles[id].position.x += 0.002;
}