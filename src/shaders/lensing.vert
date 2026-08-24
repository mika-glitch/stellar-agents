#version 450
#extension GL_ARB_separate_shader_objects : enable

// ============================================================================
// PROJECT: STELLAR-AGENTS — CORE VERTEX RESOURCE LAYOUT (VULKAN / DX12)
// Input attributes mapped from contiguous, unrolled CPU staging memory blocks.
// ============================================================================
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec4 vertexColor;

// Unified Descriptor Set 0: Static frame metrics
layout(set = 0, binding = 0) uniform FrameUniformBuffer {
    mat4 mvp;
    vec3 camPos;
    float time;
    vec2 resolution;
    vec3 camTarget;
} frameData;

// Unified Descriptor Set 0: Static field anchors
layout(set = 0, binding = 1) uniform AttractorUniformBuffer {
    vec3 holePos;
    float mass;
    float eventHorizon;
} attractorData;

// Output layout interfaces targeting the fragment raster stage
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    fragColor = vertexColor;
    fragTexCoord = (vertexPosition.xy + vec2(1.0)) * 0.5;

    // 1. GENERAL RELATIVISTIC OPTICAL PATH BENDING (ANALYTIC SCHWARZSCHILD)
    vec3 relCam = frameData.camPos - attractorData.holePos;
    vec3 relObj = vertexPosition - attractorData.holePos;
    float distObj = length(relObj);

    // Gated absorption boundary: Ray paths collapsing inside the photon sphere
    if (distObj < attractorData.eventHorizon) {
        gl_Position = frameData.mvp * vec4(attractorData.holePos, 1.0);
        return;
    }

    vec3 viewDir = normalize(vertexPosition - frameData.camPos);
    float t = dot(-relCam, viewDir);
    
    if (t < 0.0) {
        gl_Position = frameData.mvp * vec4(vertexPosition, 1.0);
        return;
    }

    vec3 closestPoint = frameData.camPos + (viewDir * t);
    float b = distance(closestPoint, attractorData.holePos);

    // Anti-infinity hardware shield preventing floating-point NaN propagation
    float effectiveImpactParameter = (b < attractorData.eventHorizon * 1.05) ? (attractorData.eventHorizon * 1.05) : b;
    float deflectionAngle = (4.0 * (attractorData.mass * 0.0015)) / effectiveImpactParameter;

    vec3 radialVector = normalize(closestPoint - attractorData.holePos);
    vec3 apparentClosestPoint = attractorData.holePos + (radialVector * effectiveImpactParameter * (1.0 + deflectionAngle));
    vec3 finalApparentWorldPosition = apparentClosestPoint + viewDir * (distObj - effectiveImpactParameter);

    // 2. DISPATCH NATIVE TRANSFORMATION MATRICES TO THE RASTERIZER
    gl_Position = frameData.mvp * vec4(finalApparentWorldPosition, 1.0);
}
