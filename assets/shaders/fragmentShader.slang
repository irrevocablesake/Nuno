struct VSInput {
    float4 position : SV_POSITION;
    float4 normal : TEXCOORD0;
};

[shader("fragment")]
float4 main(VSInput input, bool isFrontFace : SV_IsFrontFace) : SV_Target {
    float3 lightDir = normalize(float3(0.0, 0-2.0, 1.0));

    if (!isFrontFace) {
        input.normal = -input.normal;
    }

    float NdotL = saturate(dot(input.normal.xyz, lightDir));

    float3 baseColor = float3(1.0, 0.0, 0.0); 
    
    float3 ambient = baseColor * 0.2; 
    float3 diffuse = baseColor * NdotL;
    float3 specular = pow(NdotL, 32.0) * 0.5; 

    float3 color = ambient + diffuse + specular;

    return float4(color, 1.0);
}