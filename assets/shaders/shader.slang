struct VSInput {
    uint vertexID : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput main( VSInput input ) {
    VSOutput output;

    float2 position[3] = {
        float2( -0.75, -0.75 ),
        float2( 0.5, 0.75 ),
        float2( -0.75, 0.75  ),
    };

    output.position = float4(position[input.vertexID], 0.0, 1.0);
   
    return output;
}

[shader("fragment")]
float4 main(VSOutput input) : SV_Target {
    return float4( 1.0, 0.0, 0.0, 1.0 );
}