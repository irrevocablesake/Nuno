static const float3 gravity = float3( 0.0, 9.81, 0.0 );

struct Constraints {
    uint32_t a;
    uint32_t b;

    float restLength;
};

struct PushConstants {
    uint32_t offset;
    uint32_t count;
    float compliance;
    float dt;
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
RWStructuredBuffer<float> lambda;

[[vk::binding(5, 0)]]
StructuredBuffer<Constraints> constraints;

[shader("compute")]
[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadID: SV_DispatchThreadID) {
    uint threadID = dispatchThreadID.x;
    if (threadID >= push.count) return;

    uint constraintID = push.offset + threadID;
    Constraints c = constraints[constraintID];

    float w1 = mass[c.a];
    float w2 = mass[c.b];

    float3 p1 = predictedPositions[c.a].xyz;
    float3 p2 = predictedPositions[c.b].xyz;

    float3 dir = p1 - p2;
    float edgeLength = length(dir);
    if (edgeLength < 0.0001) return;

    float stretchConstraint = edgeLength - c.restLength;

    float alphaTilde = push.compliance / (push.dt * push.dt);

    float lambdaValue = lambda[constraintID];

    float3 nablaP1 = dir / edgeLength;
    float3 nablaP2 = -nablaP1;
    float denominator = (w1 * dot(nablaP1, nablaP1)) + (w2 * dot(nablaP2, nablaP2));
    if (denominator <= 0.0) return;

    denominator += alphaTilde;

    float deltaLambda = (-stretchConstraint - alphaTilde * lambdaValue ) / denominator;

    lambda[constraintID] = lambdaValue + deltaLambda;

    predictedPositions[c.a].xyz += w1 * nablaP1 * deltaLambda;
    predictedPositions[c.b].xyz += w2 * nablaP2 * deltaLambda;
}