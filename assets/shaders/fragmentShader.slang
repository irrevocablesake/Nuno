struct VSInput {
    float4 position : SV_POSITION;
    float4 normal : TEXCOORD0;
};

[shader("fragment")]
float4 main(VSInput input) : SV_Target {
    float3 lightDir = normalize(float3(1.0, 1.0, -2.0));

    float NdotL = saturate( dot( normalize(input.normal.xyz), lightDir));

    float3 baseColor = float3(0.8, 0.2, 0.2);

    float3 color = baseColor * NdotL;

    return float4( color, 1.0);
    //return float4(1.0, 0.0, 0.0, 0.0);
}
