struct VSInput {
    uint vertexID : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float4 normal : TEXCOORD0;
};

struct PushConstants {
    float dt;
    float elapsedtime;
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

[[vk::binding(8, 0)]]
StructuredBuffer<float4> normals;

[shader("vertex")]
VSOutput main( VSInput input ) {

    VSOutput output;
    output.position = mul( push.projectionMatrix, mul ( push.viewMatrix, mul(  push.modelMatrix, positions[input.vertexID])));
    output.normal = normals[input.vertexID];
   
    return output;
}