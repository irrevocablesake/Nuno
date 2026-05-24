struct AreaConstraint {
    uint32_t a;
    uint32_t b;
    uint32_t c;

    float restArea;
};

struct PushConstants {
    uint32_t offset;
    uint32_t count;

    float compliance;
    float dt;
};

[[vk::push_constant]]
PushConstants push;

[[vk::binding(1, 0)]]
RWStructuredBuffer<float4> predictedPositions;

[[vk::binding(3, 0)]]
StructuredBuffer<float> mass;

[[vk::binding(4, 0)]]
RWStructuredBuffer<float> lambda;

[[vk::binding(9, 0)]]
StructuredBuffer<AreaConstraint> constraints;

[shader("compute")]
[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    //Thread ID = One Thread per Triangle
    uint threadID = dispatchThreadID.x;
    if (threadID >= push.count) return;

    //Get Area Constraint for the triangle
    AreaConstraint c = constraints[ threadID ];

    float w1 = mass[c.a];
    float w2 = mass[c.b];
    float w3 = mass[c.c];

    //If pinned triangle skip
    if ((w1 + w2 + w3) <= 0.0) return;

    //If not pinned, use cross product to find out area of triangle
    float3 p1 = predictedPositions[c.a].xyz;
    float3 p2 = predictedPositions[c.b].xyz;
    float3 p3 = predictedPositions[c.c].xyz;

    float3 e1 = p2 - p1;
    float3 e2 = p3 - p1;

    float3 crossProduct = cross(e1, e2);
    float currentArea = 0.5 * length(crossProduct);
    float areaDifference = currentArea - c.restArea;

    float alphaTilde = push.compliance / (push.dt * push.dt);
    float lambdaValue = lambda[ threadID ];
    float3 normal = normalize(crossProduct);

    //Area gradients - which vertex to move, to have the most rate of area increase
    float3 grad1 = 0.5 * cross(normal, p3 - p2);
    float3 grad2 = 0.5 * cross(normal, p1 - p3);
    float3 grad3 = 0.5 * cross(normal, p2 - p1);

    //Compute value similar to other constraints, have to revisit this!
    float denominator = w1 * dot(grad1, grad1) + w2 * dot(grad2, grad2) + w3 * dot(grad3, grad3);
    denominator += alphaTilde;

    if (denominator <= 1e-7)
        return;

    float deltaLambda = (-areaDifference - alphaTilde * lambdaValue) / denominator;

    lambda[ threadID ] = lambdaValue + deltaLambda;

    predictedPositions[c.a].xyz += w1 * grad1 * deltaLambda;
    predictedPositions[c.b].xyz += w2 * grad2 * deltaLambda;
    predictedPositions[c.c].xyz += w3 * grad3 * deltaLambda;
}