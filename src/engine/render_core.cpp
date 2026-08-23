#include "render_core.h"
#include "math/relativity.h"
#include "raymath.h"
#include "rlgl.h" // Enforced for raw low-level OpenGL pipeline streaming
#include <cmath>

// Embed your original high-fidelity relativistic raymarching shader source
static const char* master_shader_source = R"(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;
uniform float time;
uniform vec2 resolution;
uniform vec3 camPos;
uniform vec3 camTarget;

float starHash3D(vec3 p) {
    float dotProduct = dot(p, vec3(127.1, 311.7, 74.7));
    return fract(sin(dotProduct) * 43758.5453123);
}
float hash2d(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123); }
float noise2d(vec2 p) {
    vec2 i = floor(p); vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash2d(i + vec2(0.0, 0.0)), hash2d(i + vec2(1.0, 0.0)), u.x),
               mix(hash2d(i + vec2(0.0, 1.0)), hash2d(i + vec2(1.0, 1.0)), u.x), u.y);
}
float fbm(vec2 p) {
    float v = 0.0; float a = 0.5; vec2 shift = vec2(100.0);
    mat2 rot = mat2(cos(0.6), sin(0.6), -sin(0.6), cos(0.6));
    for (int i = 0; i < 4; ++i) { v += a * noise2d(p); p = rot * p * 2.5 + shift; a *= 0.46; }
    return v;
}
void main() {
    vec2 uv = (gl_FragCoord.xy * 2.0 - resolution.xy) / resolution.y;
    vec3 forward = normalize(camTarget - camPos);
    vec3 upProjected = vec3(0.0, 1.0, 0.0);
    if (abs(forward.y) > 0.99) { upProjected = vec3(0.0, 0.0, sign(forward.y)); }
    vec3 right = normalize(cross(upProjected, forward));
    vec3 up = cross(forward, right);
    vec3 rayDir = normalize(forward - uv.x * right * 0.5 + uv.y * up * 0.5);
    vec3 rayPos = camPos;
    float mass = 0.75; float eventHorizon = 0.95;
    vec3 totalColor = vec3(0.0); bool hitSingularity = false;
    float steepness = abs(forward.y); float dynamicThickness = 0.25 + (steepness * 0.65);
    for (int step = 0; step < 160; step++) {
        float r2 = dot(rayPos, rayPos); float r = sqrt(r2);
        if (r < eventHorizon) { hitSingularity = true; break; }
        vec3 gravityDir = -rayPos / r;
        float deflection = (3.0 * mass) / (r2 * r); float stepSize = 0.04 + (r * 0.012);
        rayDir = normalize(rayDir + gravityDir * deflection * stepSize * 0.4);
        rayPos += rayDir * stepSize;
        if (abs(rayPos.y) < dynamicThickness) {
            float distToCenter = length(rayPos.xz);
            if (distToCenter > 1.05 && distToCenter < 60.0) {
                float angle = atan(rayPos.z, rayPos.x); float speed = time * 0.25; float logRadius = log(distToCenter);
                float spiralSog = 4.0 * logRadius - speed; float centralVortex = angle * 4.0 + (14.0 / (distToCenter + 0.05)) - speed * 0.5;
                vec2 vortexUV = vec2(spiralSog, centralVortex); float gasStructure = pow(fbm(vortexUV * 1.3), 1.5) * 2.5;
                float glow = 1.2 / (distToCenter - eventHorizon + 0.03);
                vec3 gasColor = vec3(0.15, 0.45, 1.0) * glow * 0.7; gasColor += vec3(0.5, 0.1, 0.9) * (gasStructure * glow * 1.5);
                float coreGlow = smoothstep(3.5, 1.05, distToCenter); gasColor += vec3(1.0, 1.0, 1.0) * (coreGlow * glow * 2.8);
                if (rayPos.x < 0.0) gasColor *= 1.6; else gasColor *= 0.55;
                float edgeFade = smoothstep(60.0, 12.0, distToCenter); float angleDamping = mix(1.0, 0.35, steepness);
                float volumeFade = smoothstep(dynamicThickness, 0.0, abs(rayPos.y));
                totalColor += gasColor * 0.045 * 3.5 * edgeFade * angleDamping * volumeFade;
            }
        }
        float gridY = -3.2;
        if ((rayPos.y - gridY) * ((rayPos.y - rayDir.y * stepSize) - gridY) < 0.0) {
            float t = (gridY - (rayPos.y - rayDir.y * stepSize)) / rayDir.y;
            vec3 gridIntersection = (rayPos - rayDir * stepSize) + rayDir * t;
            float distToGridCenter = length(gridIntersection.xz);
            if (distToGridCenter < 80.0) {
                vec2 gridCoord = fract(gridIntersection.xz * 0.8); float lineThickness = 0.03;
                if (gridCoord.x < lineThickness || gridCoord.y < lineThickness) {
                    float fadeWithDistance = smoothstep(80.0, 8.0, distToGridCenter); totalColor += vec3(0.3, 0.45, 0.6) * fadeWithDistance * 0.18;
                }
            }
        }
    }
    if (!hitSingularity) {
        vec3 finalStarDir = floor(rayDir * 500.0); float starNoise = starHash3D(finalStarDir);
        if (starNoise > 0.9935) {
            float starIntensity = smoothstep(0.9935, 1.0, starNoise); float twinkle = sin(time * 3.5 + starNoise * 1000.0) * 0.4 + 0.6;
            totalColor += vec3(starIntensity * twinkle * 2.2);
        }
    }
    if (hitSingularity) { finalColor = vec4(0.0, 0.0, 0.0, 1.0); }
    else { totalColor = totalColor / (totalColor + vec3(1.0)); totalColor = pow(totalColor, vec3(0.85)); finalColor = vec4(totalColor, 1.0); }
}
)";

stellar_agents::RenderCore::RenderCore() noexcept {
    accretion_shader = LoadShaderFromMemory(nullptr, master_shader_source);
    uniform_time = GetShaderLocation(accretion_shader, "time");
    uniform_resolution = GetShaderLocation(accretion_shader, "resolution");
    uniform_cam_pos = GetShaderLocation(accretion_shader, "camPos");
    uniform_cam_target = GetShaderLocation(accretion_shader, "camTarget");
}

stellar_agents::RenderCore::~RenderCore() {
    UnloadShader(accretion_shader);
}

void stellar_agents::RenderCore::draw_composite_scene(const Camera3D& p_camera, const EnvironmentMatrix& p_matrix) const noexcept {
    const float w = static_cast<float>(GetScreenWidth());
    const float h = static_cast<float>(GetScreenHeight());
    const Vector2 render_res = { w, h };
    const float current_time = static_cast<float>(GetTime());

    const Vector3 cached_c_pos = p_camera.position;
    const Vector3 cached_c_target = p_camera.target;

    // --- STAGE 1: EXECUTE BACKGROUND SHADER ---
    SetShaderValue(accretion_shader, uniform_time, &current_time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(accretion_shader, uniform_resolution, &render_res, SHADER_UNIFORM_VEC2);
    SetShaderValue(accretion_shader, uniform_cam_pos, &cached_c_pos, SHADER_UNIFORM_VEC3);
    SetShaderValue(accretion_shader, uniform_cam_target, &cached_c_target, SHADER_UNIFORM_VEC3);

    BeginBlendMode(BLEND_ALPHA);
    BeginShaderMode(accretion_shader);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();
    EndBlendMode();

    // --- STAGE 2: MATHEMATIK-FREIES BULK-BUFFER-STREAMING ---
    const auto& dataset = p_matrix.get_read_buffer();
    const auto& v_cache = p_matrix.get_vertex_buffer(); // Pre-calculated in thread pool
    const uint64_t entity_count = p_matrix.get_capacity();

    rlBegin(RL_LINES); // Direct zero-overhead hardware blit pass
    for (uint64_t i = 0; i < entity_count; ++i) {
        if (!dataset[i].is_active) continue;

        rlColor4ub(dataset[i].color.r, dataset[i].color.g, dataset[i].color.b, dataset[i].color.a);

        // Zero math calculation on main thread: Pull pre-sorted lines from cache
        const Vector3& vStart = v_cache[i * 2];
        const Vector3& vEnd = v_cache[i * 2 + 1];

        rlVertex3f(vStart.x, vStart.y, vStart.z);
        rlVertex3f(vEnd.x, vEnd.y, vEnd.z);
    }
    rlEnd();
}
